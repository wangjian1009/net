#include <assert.h>
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_cli_underline_i.h"

int net_ssl_cli_underline_init(net_endpoint_t base_underline) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_underline);
    undline->m_ssl_endpoint = NULL;
    return 0;
}

void net_ssl_cli_underline_fini(net_endpoint_t base_underline) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_underline);

    if (undline->m_ssl_endpoint) {
        assert(undline->m_ssl_endpoint->m_underline == base_underline);
        undline->m_ssl_endpoint->m_underline = NULL;
    }
    
    undline->m_ssl_endpoint = NULL;
}

int net_ssl_cli_underline_input(net_endpoint_t base_underline) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_underline);
    assert(undline->m_ssl_endpoint);
    assert(undline->m_ssl_endpoint->m_underline == base_underline);
    
    return 0;
}

int net_ssl_cli_underline_on_state_change(net_endpoint_t base_underline, net_endpoint_state_t from_state) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_underline);
    assert(undline->m_ssl_endpoint);
    assert(undline->m_ssl_endpoint->m_underline == base_underline);

    net_endpoint_t base_endpoint = net_endpoint_from_data(undline->m_ssl_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    switch(net_endpoint_state(base_underline)) {
    case net_endpoint_state_resolving:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_resolving) != 0) return -1;
        break;
    case net_endpoint_state_connecting:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        break;
    case net_endpoint_state_established:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        if (endpoint->m_state == net_ssl_cli_endpoint_ssl_init) {
            endpoint->m_state = net_ssl_cli_endpoint_ssl_handshake;
            if (net_ssl_cli_endpoint_handshake_start(base_endpoint, endpoint) != 0) return -1;
        }
        break;
    case net_endpoint_state_logic_error:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source(base_underline),
            net_endpoint_error_no(base_underline),
            net_endpoint_error_msg(base_underline));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) return -1;
        break;
    case net_endpoint_state_network_error:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source(base_underline),
            net_endpoint_error_no(base_underline),
            net_endpoint_error_msg(base_underline));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
        break;
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source_network,
            net_endpoint_network_errno_logic,
            "underline ep state error");
        return -1;
    }
    
    return 0;
}

net_protocol_t
net_ssl_cli_underline_protocol_create(net_schedule_t schedule, const char * name) {
    return net_protocol_create(
        schedule,
        name,
        /*protocol*/
        0, NULL, NULL,
        /*endpoint*/
        sizeof(struct net_ssl_cli_undline),
        net_ssl_cli_underline_init,
        net_ssl_cli_underline_fini,
        net_ssl_cli_underline_input,
        net_ssl_cli_underline_on_state_change,
        NULL);
}
