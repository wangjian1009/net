#include "assert.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_strings.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_dns_ns_cli_endpoint_i.h"
#include "net_dns_ns_cli_protocol_i.h"
#include "net_dns_source_i.h"
#include "net_dns_ns_pro.h"
#include "net_dns_task_ctx_i.h"

net_dns_ns_cli_endpoint_t
net_dns_ns_cli_endpoint_create(net_dns_source_t source, net_driver_t driver, net_dns_task_ctx_t task_ctx) {
    net_dns_manage_t manage = source->m_manage;
    
    net_endpoint_t base_endpoint =
        net_endpoint_create(
            driver, net_protocol_from_data(source->m_manage->m_protocol_dns_ns_cli));
    if (base_endpoint == NULL) {
        CPE_ERROR(
            manage->m_em, "dns-cli: %s: create ns-cli-endpoint fail!",
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), source));
        return NULL;
    }

    net_dns_ns_cli_endpoint_t dns_cli = net_endpoint_protocol_data(base_endpoint);
    
    if (net_dns_ns_parser_init(&dns_cli->m_parser, manage, source) != 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: %s: init parser fail!",
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), source));
        net_endpoint_free(base_endpoint);
        return NULL;
    }

    dns_cli->m_task_ctx = task_ctx;
    
    return dns_cli;
}

void net_dns_ns_cli_endpoint_free(net_dns_ns_cli_endpoint_t dns_cli) {
    net_endpoint_free(dns_cli->m_endpoint);
}

net_dns_ns_cli_endpoint_t net_dns_ns_cli_endpoint_get(net_endpoint_t base_endpoint) {
    return net_endpoint_data(base_endpoint);
}

int net_dns_ns_cli_endpoint_init(net_endpoint_t base_endpoint) {
    net_dns_ns_cli_endpoint_t dns_cli = net_endpoint_protocol_data(base_endpoint);
    dns_cli->m_endpoint = base_endpoint;
    bzero(&dns_cli->m_parser, sizeof(dns_cli->m_parser));
    dns_cli->m_task_ctx = NULL;
    return 0;
}

void net_dns_ns_cli_endpoint_fini(net_endpoint_t base_endpoint) {
    net_dns_ns_cli_endpoint_t dns_cli = net_endpoint_protocol_data(base_endpoint);
    net_dns_ns_parser_fini(&dns_cli->m_parser);
}

static int net_dns_ns_cli_process_data(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf)
{
    net_dns_ns_cli_endpoint_t dns_cli = net_endpoint_protocol_data(base_endpoint);
    net_dns_ns_cli_protocol_t dns_protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_dns_manage_t manage = dns_protocol->m_manage;

    while(1) {
        uint32_t input_sz = net_endpoint_buf_size(from_ep, from_buf);
        void * input;
        if (net_endpoint_buf_peak_with_size(from_ep, from_buf, input_sz, &input) != 0) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: <-- get input buf fail, sz=%d!",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), base_endpoint), input_sz);
            return -1;
        }
    
        if (input_sz < 2) return 0;

        uint16_t req_sz;
        CPE_COPY_NTOH16(&req_sz, input);

        uint32_t all_req_sz = req_sz + (uint32_t)sizeof(req_sz);
        if (all_req_sz > input_sz) return 0;

        input = ((char*)input) + sizeof(req_sz);
            
        net_dns_ns_parser_reset(&dns_cli->m_parser);
        int rv = net_dns_ns_parser_input(&dns_cli->m_parser, input, req_sz);
        if (rv < 0) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: <-- parse data fail, req-sz=%d, parser.state=%d, parser.r-pos=%d!",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), base_endpoint),
                req_sz, dns_cli->m_parser.m_state, dns_cli->m_parser.m_r_pos);
            return -1;
        }

        if (dns_cli->m_parser.m_state != net_dns_ns_parser_completed) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: <-- parse data complete, state error, state=%d, req-sz=%d, parser.state=%d, parser.r-pos=%d!",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), base_endpoint),
                dns_cli->m_parser.m_state, req_sz, dns_cli->m_parser.m_state, dns_cli->m_parser.m_r_pos);
            return -1;
        }

        if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
            CPE_INFO(
                manage->m_em, "dns-cli: %s: tcp <-- (%d/%d) %s",
                net_endpoint_dump(&manage->m_data_buffer, base_endpoint),
                rv, req_sz,
                net_dns_ns_req_dump(manage, net_dns_manage_tmp_buffer(manage), input, (uint32_t)rv));
        }

        net_endpoint_buf_consume(from_ep, from_buf, all_req_sz);

        if (dns_cli->m_task_ctx) {
            if (dns_cli->m_parser.m_record_count > 0) {
                net_dns_task_ctx_set_success(dns_cli->m_task_ctx);
            }
            else {
                net_dns_task_ctx_set_empty(dns_cli->m_task_ctx);
            }
            dns_cli->m_task_ctx = NULL;
        }

        return 0;
    }

    return 0;
}
    
int net_dns_ns_cli_endpoint_forward(net_endpoint_t base_endpoint, net_endpoint_t from) {
    return net_dns_ns_cli_process_data(base_endpoint, from, net_ep_buf_forward);
}

int net_dns_ns_cli_endpoint_input(net_endpoint_t base_endpoint) {
    return net_dns_ns_cli_process_data(base_endpoint, base_endpoint, net_ep_buf_read);
}

int net_dns_ns_cli_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    if (!net_endpoint_is_active(base_endpoint)) {
        net_dns_ns_cli_endpoint_t dns_cli = net_endpoint_protocol_data(base_endpoint);
        
        if (dns_cli->m_task_ctx) {
            net_dns_ns_cli_endpoint_t dns_cli = net_endpoint_protocol_data(base_endpoint);
            /* net_dns_ns_cli_protocol_t dns_protocol = net_protocol_data(net_endpoint_protocol(base_endpoint)); */
            /* net_dns_manage_t manage = dns_protocol->m_manage; */
            
            net_dns_task_ctx_set_error(dns_cli->m_task_ctx);
            dns_cli->m_task_ctx = NULL;
        }
    }
    
    return 0;
}

int net_dns_ns_cli_endpoint_send(
    net_dns_ns_cli_endpoint_t dns_cli, net_address_t address, void const * buf, uint16_t buf_size)
{
    net_dns_manage_t manage = dns_cli->m_parser.m_manage;

    if (net_endpoint_remote_address(dns_cli->m_endpoint) == NULL) {
        if (net_endpoint_set_remote_address(dns_cli->m_endpoint, address, 0) != 0) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: --> set remote address fail",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
            return -1;
        }
    }

    if (!net_endpoint_is_active(dns_cli->m_endpoint)) {
        if (net_endpoint_connect(dns_cli->m_endpoint) != 0) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: --> connect fail",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
            return -1;
        }
    }
    
    net_endpoint_t other = net_endpoint_other(dns_cli->m_endpoint);
    if (other) {
        if (!net_endpoint_is_active(other)) {
            if (net_endpoint_connect(other) != 0) {
                CPE_ERROR(
                    manage->m_em, "dns-cli: %s: --> chanel connect fail",
                    net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
                return -1;
            }

            if (net_endpoint_direct(other, net_endpoint_remote_address(dns_cli->m_endpoint)) != 0) {
                CPE_ERROR(
                    manage->m_em, "dns-cli: %s: --> chanel direct fail",
                    net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
                net_endpoint_set_state(other, net_endpoint_state_logic_error);
                return -1;
            }
        }

        if (net_endpoint_buf_append(dns_cli->m_endpoint, net_ep_buf_forward, buf, buf_size) < 0
            || net_endpoint_forward(dns_cli->m_endpoint) != 0)
        {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: --> fbuf append fail",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
            return -1;
        }
    }
    else {
        if (!net_endpoint_is_active(dns_cli->m_endpoint)) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: --> not active!",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
            return -1;
        }
    
        if (net_endpoint_buf_append(dns_cli->m_endpoint, net_ep_buf_write, buf, buf_size) < 0) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: --> wbuf append fail",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
            return -1;
        }
    }

    if (net_endpoint_protocol_debug(dns_cli->m_endpoint) >= 2) {
        CPE_INFO(
            manage->m_em, "dns-cli: %s: tcp --> (%d)%s",
            net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint),
            buf_size - 2,
            net_dns_ns_req_dump(manage, &manage->m_data_buffer, ((uint8_t const *)buf) + 2, buf_size - 2));
    }
    
    return 0;
}
