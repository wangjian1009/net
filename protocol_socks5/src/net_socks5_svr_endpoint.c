#include "assert.h"
#include "cpe/pal/pal_socket.h"
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

static int net_socks5_svr_send_socks5_connect_response(net_endpoint_t endpoint, net_address_t address);
static int net_socks5_svr_send_socks4_connect_response(net_endpoint_t endpoint, net_address_t address);

int net_socks5_svr_endpoint_init(net_endpoint_t endpoint) {
    net_socks5_svr_endpoint_t socks5_svr = net_endpoint_protocol_data(endpoint);
    socks5_svr->m_stage = net_socks5_svr_endpoint_stage_init;
    socks5_svr->m_on_connect_fun = NULL;
    socks5_svr->m_on_connect_ctx = NULL;
    return 0;
}

void net_socks5_svr_endpoint_fini(net_endpoint_t endpoint) {
}

void net_socks5_svr_endpoint_set_connect_fun(
    net_socks5_svr_endpoint_t ss_ep,
    net_socks5_svr_connect_fun_t on_connect_fon, void * on_connect_ctx)
{
    ss_ep->m_on_connect_fun = on_connect_fon;
    ss_ep->m_on_connect_ctx = on_connect_ctx;
}

#define net_socks5_svr_endpoint_check_read(__sz) \
    if (net_endpoint_buf_peak_with_size(endpoint, net_ep_buf_read, (__sz), &data) != 0) return -1; \
    if (data == NULL) break

#define net_socks5_svr_endpoint_verify_version()                        \
    if (*(uint8_t*)data != SOCKS5_SVERSION                              \
        && *(uint8_t*)data != SOCKS4_SVERSION) {                        \
        CPE_ERROR(                                                      \
            em, "ss: %s: version %d not support!",                      \
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint), \
            *(uint8_t*)data);                                           \
        return -1;                                                      \
    }                                                                   \

int net_socks5_svr_endpoint_input(net_endpoint_t endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    error_monitor_t em = net_schedule_em(schedule);
    net_socks5_svr_t socks5_svr = net_protocol_data(net_endpoint_protocol(endpoint));
    net_socks5_svr_endpoint_t ss_ep = net_endpoint_protocol_data(endpoint);
    void * data;

    uint32_t left_size;
    for(left_size = net_endpoint_buf_size(endpoint, net_ep_buf_read);
        left_size > 0;
        left_size = net_endpoint_buf_size(endpoint, net_ep_buf_read)
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
            case net_endpoint_state_logic_error:
                CPE_ERROR(
                    em, "ss: %s: state in error!",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
                return -1;
            default:
                break;
            }
            
            if (net_endpoint_buf_append_from_self(endpoint, net_ep_buf_forward, net_ep_buf_read, 0) != 0) return -1;
            if (net_endpoint_forward(endpoint) != 0) return -1;
            return 0;
        }
        else if (ss_ep->m_stage == net_socks5_svr_endpoint_stage_init) {
            /*验证版本 */
            net_socks5_svr_endpoint_check_read(1);
            net_socks5_svr_endpoint_verify_version();

            if (*(uint8_t*)data == SOCKS4_SVERSION) {
                ss_ep->m_stage = net_socks5_svr_endpoint_stage_handshake;
            }
            else {
                assert(*(uint8_t*)data == SOCKS5_SVERSION);

                /*读取 socks5_select_request*/
                net_socks5_svr_endpoint_check_read(sizeof(struct socks5_select_request));
                uint32_t select_request_len = sizeof(struct socks5_select_request) + ((struct socks5_select_request *)data)->nmethods;

                net_socks5_svr_endpoint_check_read(select_request_len);
                struct socks5_select_request * select_request = (struct socks5_select_request *)data;

                struct socks5_select_response response;
                response.ver    = SOCKS5_SVERSION;
                response.method = SOCKS5_METHOD_UNACCEPTABLE;
                uint8_t i;
                for (i = 0; i < select_request->nmethods; i++) {
                    if (select_request->methods[i] == SOCKS5_METHOD_NOAUTH) {
                        response.method = SOCKS5_METHOD_NOAUTH;
                        break;
                    }
                }

                if (net_endpoint_buf_append(endpoint, net_ep_buf_write,  &response, sizeof(response)) != 0) {
                    CPE_ERROR(
                        em, "ss: %s: send select response fail!",
                        net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
                    return -1;
                }

                ss_ep->m_stage = net_socks5_svr_endpoint_stage_handshake;
                net_endpoint_buf_consume(endpoint, net_ep_buf_read, select_request_len);
            }
        }
        else if (ss_ep->m_stage == net_socks5_svr_endpoint_stage_handshake
                 || ss_ep->m_stage == net_socks5_svr_endpoint_stage_parse
                 || ss_ep->m_stage == net_socks5_svr_endpoint_stage_sni)
        {
            net_socks5_svr_endpoint_check_read(2);
            net_socks5_svr_endpoint_verify_version();

            uint8_t version = ((uint8_t*)data)[0];
            uint8_t cmd = ((uint8_t*)data)[1];
            
            if (cmd == SOCKS5_CMD_CONNECT) {
                net_address_t address = NULL;
                uint32_t used_len = 0;
                const char * username = NULL;

                if (version == SOCKS4_SVERSION) {
                    uint32_t peak_data_size;
                    data = net_endpoint_buf_peak(endpoint, net_ep_buf_read, &peak_data_size);
                    assert(data);

                    uint32_t pos;
                    for(pos = sizeof(struct socks4_connect_request);
                        pos < peak_data_size;
                        ++pos)
                    {
                        if (((uint8_t*)data)[pos] == 0) break;
                    }
                    if (pos >= peak_data_size) break; /* 数据不足 */
                    
                    used_len = pos + 1;
                    username = ((uint8_t*)data) + sizeof(struct socks4_connect_request);
                    struct socks4_connect_request * request = data;
                    struct sockaddr_in in_addr;
                    in_addr.sin_family = AF_INET;
                    memcpy(&in_addr.sin_addr, &request->dst_ip, sizeof(in_addr.sin_addr));
                    memcpy(&in_addr.sin_port, &request->dst_port, sizeof(in_addr.sin_port));
                           
                    address = net_address_create_from_sockaddr(schedule, (struct sockaddr *)&in_addr, sizeof(in_addr));

                    /* Fake reply */
                    if (ss_ep->m_stage == net_socks5_svr_endpoint_stage_handshake) {
                        if (net_socks5_svr_send_socks4_connect_response(endpoint, NULL) != 0) return -1;
                        ss_ep->m_stage = net_socks5_svr_endpoint_stage_parse;
                    }
                }
                else {
                    assert(version == SOCKS5_SVERSION);
                    net_socks5_svr_endpoint_check_read(sizeof(struct socks5_connect_request));
                    
                    struct socks5_connect_request * request = data;
                
                    /* Fake reply */
                    if (ss_ep->m_stage == net_socks5_svr_endpoint_stage_handshake) {
                        if (net_socks5_svr_send_socks5_connect_response(endpoint, NULL) != 0) return -1;
                        ss_ep->m_stage = net_socks5_svr_endpoint_stage_parse;
                    }

                    if (request->atyp == SOCKS5_ATYPE_IPV4) {
                        used_len = sizeof(struct socks5_connect_request) + 4 + 2;
                        net_socks5_svr_endpoint_check_read(used_len);

                        struct sockaddr_in in_addr;
                        in_addr.sin_family = AF_INET;
                        memcpy(&in_addr.sin_addr, request + 1, 4);
                        memcpy(&in_addr.sin_port, ((char*)(request + 1)) + 4, 2);
                           
                        address = net_address_create_from_sockaddr(schedule, (struct sockaddr *)&in_addr, sizeof(in_addr));
                    }
                    else if (request->atyp == SOCKS5_ATYPE_DOMAIN) {
                        net_socks5_svr_endpoint_check_read(sizeof(struct socks5_connect_request) + 1);
                    
                        uint8_t name_len = *(uint8_t *)(request + 1);
                        used_len = sizeof(struct socks5_connect_request) + 1 + name_len + 2;
                        net_socks5_svr_endpoint_check_read(used_len);

                        uint16_t port;
                        memcpy(&port, ((uint8_t*)(request + 1)) + 1 + name_len, 2);
                        port = ntohs(port);

                        address = net_address_create_domain_with_len(
                            schedule, ((uint8_t*)(request + 1)) + 1, name_len, port, NULL);
                    }
                    else if (request->atyp == SOCKS5_ATYPE_IPV6) {
                        used_len = sizeof(struct socks5_connect_request) + 16 + 2;
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
                }

                if (address == NULL) return -1;

                ss_ep->m_stage = net_socks5_svr_endpoint_stage_stream;
                net_endpoint_buf_consume(endpoint, net_ep_buf_read, used_len);

                if (net_endpoint_protocol_debug(endpoint) >= 2) {
                    CPE_INFO(
                        em, "ss: %s: request connect to %s!",
                        net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint),
                        net_address_dump(&socks5_svr->m_tmp_buffer, address));
                }

                if (ss_ep->m_on_connect_fun &&
                    ss_ep->m_on_connect_fun(ss_ep->m_on_connect_ctx, endpoint, address, 1) != 0)
                {
                    net_address_free(address);
                    return -1;
                }
            }
            else if (cmd == SOCKS5_CMD_BIND) {
                CPE_ERROR(
                    em, "ss: %s: not support socks5 cmd bind!",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
                return -1;
            }
            else if (cmd == SOCKS5_CMD_UDP_ASSOCIATE) {
				CPE_ERROR(
					em, "ss: %s: not support socks5 cmd udp associate!",
					net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
				return -1;
            }
            else {
                CPE_ERROR(
                    em, "ss: %s: unknown socks5 cmd %d!",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint), cmd);
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

int net_socks5_svr_endpoint_forward(net_endpoint_t endpoint, net_endpoint_t from) {
    return net_endpoint_buf_append_from_other(endpoint, net_ep_buf_write, from, net_ep_buf_forward, 0);
}

static int net_socks5_svr_send_socks5_connect_response(net_endpoint_t endpoint, net_address_t address) {
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    struct socks5_connect_response * response = NULL;
    uint32_t buf_len = 0;
    mem_buffer_t tmp_buffer = net_schedule_tmp_buffer(schedule);

    mem_buffer_clear_data(tmp_buffer);
    
    if (address == NULL) {
        buf_len = sizeof(struct socks5_connect_response) + 4 + 2;
        response = mem_buffer_alloc(tmp_buffer, buf_len);
        bzero(response, buf_len);
        response->atyp = SOCKS5_ATYPE_IPV4;
    }
    else {
        switch(net_address_type(address)) {
        case net_address_ipv4:
            buf_len = sizeof(struct socks5_connect_response) + 4 + 2;
            response = mem_buffer_alloc(tmp_buffer, buf_len);
            memcpy(response + 1, net_address_data(address), 4);
            response->atyp = SOCKS5_ATYPE_IPV4;
            break;
        case net_address_ipv6:
            buf_len = sizeof(struct socks5_connect_response) + 16 + 2;
            response = mem_buffer_alloc(tmp_buffer, buf_len);
            memcpy(response + 1, net_address_data(address), 16);
            response->atyp = SOCKS5_ATYPE_IPV6;
            break;
        case net_address_domain: {
            const char * url = net_address_data(address);
            uint16_t url_len = (uint16_t)strlen(url);
            buf_len = sizeof(struct socks5_connect_response) + 1 + url_len + 2;
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
    
    if (net_endpoint_buf_append(endpoint, net_ep_buf_write,  response, buf_len) != 0) {
        CPE_ERROR(
            net_schedule_em(schedule), "ss: %s: send response %d fail!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint), buf_len);
        return -1;
    }

    return 0;
}

static int net_socks5_svr_send_socks4_connect_response(net_endpoint_t endpoint, net_address_t address) {
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    uint32_t buf_size = sizeof(struct socks4_connect_response);
    struct socks4_connect_response * response = net_endpoint_buf_alloc(endpoint, &buf_size);
    if (response == NULL) {
        CPE_ERROR(
            net_schedule_em(schedule),
            "ss: %s: send socks4 connect response: alloc buf fail!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
        return -1;
    }

    response->ver = 0x00;
    response->rep = 0x5A; /*允许转发 */
        
    if (address == NULL) {
        response->dst_port = 0;
        response->dst_ip = 0;
    }
    else {
        switch(net_address_type(address)) {
        case net_address_ipv4:
            memcpy(&response->dst_ip, net_address_data(address), 4);
            break;
        case net_address_ipv6:
            CPE_ERROR(
                net_schedule_em(schedule),
                "ss: %s: send socks4 connect response: not support ipv6 address!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
            net_endpoint_buf_release(endpoint);
            return -1;
        case net_address_domain:
            CPE_ERROR(
                net_schedule_em(schedule),
                "ss: %s: send socks4 connect response: not support domain address!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
            net_endpoint_buf_release(endpoint);
            return -1;
        }

        uint16_t port = htons(net_address_port(address));
        memcpy(&response->dst_port, &port, 2);
    }
    
    if (net_endpoint_buf_supply(endpoint, net_ep_buf_write, sizeof(struct socks4_connect_response)) != 0) {
        CPE_ERROR(
            net_schedule_em(schedule), "ss: %s: send socks4 connect response fail!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
        return -1;
    }

    return 0;
}
