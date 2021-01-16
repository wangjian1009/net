#include <assert.h>
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ssl_cli_underline_i.h"

int net_ssl_cli_underline_init(net_endpoint_t base_endpoint) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_endpoint);
    undline->m_ssl_endpoint = NULL;
    return 0;
}

void net_ssl_cli_underline_fini(net_endpoint_t base_endpoint) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_endpoint);
    undline->m_ssl_endpoint = NULL;
}

int net_ssl_cli_underline_input(net_endpoint_t base_endpoint) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_endpoint);
    assert(undline->m_ssl_endpoint);
    assert(undline->m_ssl_endpoint->m_underline == base_endpoint);
    
    return 0;
}

int net_ssl_cli_underline_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
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
