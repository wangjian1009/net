#include "cpe/pal/pal_strings.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_dns_ns_cli_endpoint_i.h"
#include "net_dns_ns_cli_protocol_i.h"
#include "net_dns_source_i.h"
#include "net_dns_ns_pro.h"

net_dns_ns_cli_endpoint_t
net_dns_ns_cli_endpoint_create(net_dns_source_t source, net_driver_t driver) {
    net_dns_manage_t manage = source->m_manage;
    
    net_endpoint_t base_endpoint =
        net_endpoint_create(
            driver, net_endpoint_outbound, net_protocol_from_data(source->m_manage->m_protocol_dns_ns_cli));
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
    return 0;
}

void net_dns_ns_cli_endpoint_fini(net_endpoint_t base_endpoint) {
    net_dns_ns_cli_endpoint_t dns_cli = net_endpoint_protocol_data(base_endpoint);
    return net_dns_ns_parser_fini(&dns_cli->m_parser);
}

int net_dns_ns_cli_endpoint_input(net_endpoint_t base_endpoint) {
    net_dns_ns_cli_endpoint_t dns_cli = net_endpoint_protocol_data(base_endpoint);
    net_dns_ns_cli_protocol_t dns_protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_dns_manage_t manage = dns_protocol->m_manage;
    
    uint32_t input_sz = net_endpoint_rbuf_size(base_endpoint);
    void * input;
    if (net_endpoint_rbuf(base_endpoint, input_sz, &input) != 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: %s: <-- get input buf fail, sz=%d!",
            net_endpoint_dump(net_dns_manage_tmp_buffer(manage), base_endpoint), input_sz);
        return -1;
    }

    int rv = net_dns_ns_parser_input(&dns_cli->m_parser, input, input_sz);
    if (rv < 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: %s: <-- parse data fail, input-sz=%d, parser.state=%d, parser.r-pos=%d!",
            net_endpoint_dump(net_dns_manage_tmp_buffer(manage), base_endpoint),
            input_sz, dns_cli->m_parser.m_state, dns_cli->m_parser.m_r_pos);
        return -1;
    }

    if (rv == 0) return 0;
    
    if (manage->m_debug) {
        CPE_INFO(
            manage->m_em, "dns-cli: %s: tcp <-- (%d/%d) %s",
            net_endpoint_dump(&manage->m_data_buffer, base_endpoint),
            rv, input_sz,
            net_dns_ns_req_dump(manage, net_dns_manage_tmp_buffer(manage), input, (uint32_t)rv));
    }

    net_endpoint_rbuf_consume(base_endpoint, (uint32_t)rv);

    return 0;
}

int net_dns_ns_cli_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state) {
    return 0;
}

int net_dns_ns_cli_endpoint_send(
    net_dns_ns_cli_endpoint_t dns_cli, net_address_t address, void const * buf, uint16_t buf_size)
{
    net_dns_manage_t manage = dns_cli->m_parser.m_manage;

    if (net_endpoint_remote_address(dns_cli->m_endpoint) == NULL) {
        if (net_endpoint_set_remote_address(dns_cli->m_endpoint, address, 0) != 0) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: set endpoint remote address fail",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
            return -1;
        }
    }
    
    if (net_endpoint_is_active(dns_cli->m_endpoint)) {
        if (net_endpoint_connect(dns_cli->m_endpoint) != 0) {
            CPE_ERROR(
                manage->m_em, "dns-cli: %s: connect fail",
                net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
            return -1;
        }
    }
    
    if (net_endpoint_wbuf_append(dns_cli->m_endpoint, buf, buf_size) < 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: %s: send data fail",
            net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint));
        return -1;
    }

    if (manage->m_debug >= 2) {
        CPE_INFO(
            manage->m_em, "dns-cli: %s: tcp --> %s",
            net_endpoint_dump(net_dns_manage_tmp_buffer(manage), dns_cli->m_endpoint),
            net_dns_ns_req_dump(manage, &manage->m_data_buffer, buf, buf_size));
    }
    
    return 0;
}
