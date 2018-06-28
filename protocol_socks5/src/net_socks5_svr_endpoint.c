#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/buffer.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_socks5_svr_endpoint_i.h"
#include "net_socks5_svr_i.h"
#include "net_socks5_pro.h"

static int net_socks5_svr_send_socks5_response(net_endpoint_t endpoint, net_address_t address);
    
int net_socks5_svr_endpoint_init(net_endpoint_t endpoint) {
    net_socks5_svr_endpoint_t socks5_svr = net_endpoint_protocol_data(endpoint);
    socks5_svr->m_stage = net_socks5_svr_endpoint_stage_init;
    return 0;
}

void net_socks5_svr_endpoint_fini(net_endpoint_t endpoint) {
}

#define net_socks5_svr_endpoint_check_read(__sz) \
    if (net_endpoint_rbuf(endpoint, (__sz), &data) != 0) return -1;  \
    if (data == NULL) break

#define net_socks5_svr_endpoint_verify_version()                      \
    if (*(uint8_t*)data != SOCKS5_SVERSION) {                           \
        CPE_ERROR(                                                      \
            em, "ss: %s: version mismatch!",                            \
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint)); \
        return -1;                                                      \
    }                                                                   \

int net_socks5_svr_endpoint_input(net_endpoint_t endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    error_monitor_t em = net_schedule_em(schedule);
    net_socks5_svr_t socks5_svr = net_protocol_data(net_endpoint_protocol(endpoint));
    net_socks5_svr_endpoint_t ss_ep = net_endpoint_protocol_data(endpoint);
    void * data;

    uint32_t left_size;
    for(left_size = net_endpoint_rbuf_size(endpoint);
        left_size > 0;
        left_size = net_endpoint_rbuf_size(endpoint)
        )
    {
        if (ss_ep->m_stage == net_socks5_svr_endpoint_stage_stream) {
            if (net_endpoint_link(endpoint) == NULL) {
                CPE_ERROR(
                    em, "ss: %s: no link!",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
                return -1;
            }

            switch(net_endpoint_state(endpoint)) {
            case net_endpoint_state_error:
                CPE_ERROR(
                    em, "ss: %s: state in error!",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
                return -1;
            default:
                break;
            }
            
            if (net_endpoint_fbuf_append_from_rbuf(endpoint, 0) != 0) return -1;
            if (net_endpoint_forward(endpoint) != 0) return -1;
            return 0;
        }
        else if (ss_ep->m_stage == net_socks5_svr_endpoint_stage_init) {
            /*验证版本 */
            net_socks5_svr_endpoint_check_read(1);
            net_socks5_svr_endpoint_verify_version();

            /*读取 method_select_request*/
            net_socks5_svr_endpoint_check_read(sizeof(struct method_select_request));
            uint32_t select_request_len = sizeof(struct method_select_request) + ((struct method_select_request *)data)->nmethods;

            net_socks5_svr_endpoint_check_read(select_request_len);
            struct method_select_request * select_request = (struct method_select_request *)data;

            struct method_select_response response;
            response.ver    = SOCKS5_SVERSION;
            response.method = SOCKS5_METHOD_UNACCEPTABLE;
            uint8_t i;
            for (i = 0; i < select_request->nmethods; i++) {
                if (select_request->methods[i] == SOCKS5_METHOD_NOAUTH) {
                    response.method = SOCKS5_METHOD_NOAUTH;
                    break;
                }
            }

            if (net_endpoint_wbuf_append(endpoint, &response, sizeof(response)) != 0) {
                CPE_ERROR(
                    em, "ss: %s: send select response fail!",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
                return -1;
            }

            ss_ep->m_stage = net_socks5_svr_endpoint_stage_handshake;
            net_endpoint_rbuf_consume(endpoint, select_request_len);
        }
        else if (ss_ep->m_stage == net_socks5_svr_endpoint_stage_handshake
                 || ss_ep->m_stage == net_socks5_svr_endpoint_stage_parse
                 || ss_ep->m_stage == net_socks5_svr_endpoint_stage_sni)
        {
            net_socks5_svr_endpoint_check_read(sizeof(struct socks5_request));
            net_socks5_svr_endpoint_verify_version();
            
            struct socks5_request * request = data;
            
            if (request->cmd == SOCKS5_CMD_CONNECT) {
                /* Fake reply */
                if (ss_ep->m_stage == net_socks5_svr_endpoint_stage_handshake) {
                    if (net_socks5_svr_send_socks5_response(endpoint, NULL) != 0) return -1;
                    ss_ep->m_stage = net_socks5_svr_endpoint_stage_parse;
                }

                net_address_t address = NULL;
                uint32_t used_len = 0;
                
                if (request->atyp == SOCKS5_ATYPE_IPV4) {
                    used_len = sizeof(struct socks5_request) + 4 + 2;
                    net_socks5_svr_endpoint_check_read(used_len);

                    struct sockaddr_in in_addr;
                    in_addr.sin_family = AF_INET;
                    memcpy(&in_addr.sin_addr, request + 1, 4);
                    memcpy(&in_addr.sin_port, ((char*)(request + 1)) + 4, 2);
                           
                    address = net_address_create_from_sockaddr(schedule, (struct sockaddr *)&in_addr, sizeof(in_addr));
                }
                else if (request->atyp == SOCKS5_ATYPE_DOMAIN) {
                    net_socks5_svr_endpoint_check_read(sizeof(struct socks5_request) + 1);
                    
                    uint8_t name_len = *(uint8_t *)(request + 1);
                    used_len = sizeof(struct socks5_request) + 1 + name_len + 2;
                    net_socks5_svr_endpoint_check_read(used_len);

                    uint16_t port;
                    memcpy(&port, ((uint8_t*)(request + 1)) + 1 + name_len, 2);
                    port = ntohs(port);

                    address = net_address_create_domain_with_len(
                        schedule, ((uint8_t*)(request + 1)) + 1, name_len, port, NULL);
                }
                else if (request->atyp == SOCKS5_ATYPE_IPV6) {
                    used_len = sizeof(struct socks5_request) + 16 + 2;
                    net_socks5_svr_endpoint_check_read(used_len);

                    struct sockaddr_in6 in6_addr;
                    in6_addr.sin6_family = AF_INET6;
                    memcpy(&in6_addr.sin6_addr, request + 1, 16);
                    memcpy(&in6_addr.sin6_port, ((char*)(request + 1)) + 16, 2);
                           
                    address = net_address_create_from_sockaddr(schedule, (struct sockaddr *)&in6_addr, sizeof(in6_addr));
                }
                else {
                    CPE_ERROR(
                        em, "ss: %s: not support socks5 atype %d!",
                        net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint), request->atyp);
                    return -1;
                }

                if (address == NULL) return -1;

                ss_ep->m_stage = net_socks5_svr_endpoint_stage_stream;
                net_endpoint_rbuf_consume(endpoint, used_len);

                if (net_schedule_debug(schedule) >= 2) {
                    CPE_INFO(
                        em, "ss: %s: request connect to %s!",
                        net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint),
                        net_address_dump(&socks5_svr->m_tmp_buffer, address));
                }

                if (socks5_svr->m_dft_connect(socks5_svr->m_dft_connect_ctx, endpoint, address, 1) != 0) {
                    net_address_free(address);
                    return -1;
                }
            }
            else if (request->cmd == SOCKS5_CMD_BIND) {
                CPE_ERROR(
                    em, "ss: %s: not support socks5 cmd bind!",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
                return -1;
            }
            else if (request->cmd == SOCKS5_CMD_UDP_ASSOCIATE) {
                if (net_socks5_svr_send_socks5_response(endpoint, net_endpoint_address(endpoint)) != 0) return -1;
                net_endpoint_rbuf_consume(endpoint, sizeof(struct socks5_request));
            }
            else {
                CPE_ERROR(
                    em, "ss: %s: unknown socks5 cmd %d!",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint), request->cmd);
                return -1;
            }
        }
        else {
            CPE_ERROR(
                em, "ss: %s: in unknown stage %d",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint),
                ss_ep->m_stage);
            return -1;
        }
    }

    return 0;
}

static int net_socks5_svr_send_socks5_response(net_endpoint_t endpoint, net_address_t address) {
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    struct socks5_response * response = NULL;
    uint32_t buf_len = 0;
    mem_buffer_t tmp_buffer = net_schedule_tmp_buffer(schedule);

    mem_buffer_clear_data(tmp_buffer);
    
    if (address == NULL) {
        buf_len = sizeof(struct socks5_response) + 4 + 2;
        response = mem_buffer_alloc(tmp_buffer, buf_len);
        bzero(response, buf_len);
        response->atyp = SOCKS5_ATYPE_IPV4;
    }
    else {
        switch(net_address_type(address)) {
        case net_address_ipv4:
            buf_len = sizeof(struct socks5_response) + 4 + 2;
            response = mem_buffer_alloc(tmp_buffer, buf_len);
            memcpy(response + 1, net_address_data(address), 4);
            response->atyp = SOCKS5_ATYPE_IPV4;
            break;
        case net_address_ipv6:
            buf_len = sizeof(struct socks5_response) + 16 + 2;
            response = mem_buffer_alloc(tmp_buffer, buf_len);
            memcpy(response + 1, net_address_data(address), 16);
            response->atyp = SOCKS5_ATYPE_IPV6;
            break;
        case net_address_domain: {
            const char * url = net_address_data(address);
            uint16_t url_len = strlen(url);
            buf_len = sizeof(struct socks5_response) + 1 + url_len + 2;
            response = mem_buffer_alloc(tmp_buffer, buf_len);
            *(uint8_t*)(response + 1) = (uint8_t)url_len;
            memcpy(response + 1 + 1, url, url_len);
            response->atyp = SOCKS5_ATYPE_DOMAIN;
            break;
        }
        }

        uint16_t port = htons(net_address_port(address));
        memcpy(((char *)response) + buf_len - 2, &port, 2);
    }

    response->ver = SOCKS5_SVERSION;
    response->rep = 0;
    response->rsv = 0;
    
    if (net_endpoint_wbuf_append(endpoint, response, buf_len) != 0) {
        CPE_ERROR(
            net_schedule_em(schedule), "ss: %s: send response %d fail!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint), buf_len);
        return -1;
    }

    return 0;
}

int net_socks5_svr_endpoint_forward(net_endpoint_t endpoint, net_endpoint_t from) {
    return net_endpoint_wbuf_append_from_other(endpoint, from, 0);
}
