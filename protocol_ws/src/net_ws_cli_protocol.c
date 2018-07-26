#include "net_protocol.h"
#include "net_ws_cli_protocol_i.h"
#include "net_ws_cli_endpoint_i.h"

static int net_ws_cli_protocol_init(net_protocol_t protocol);
static void net_ws_cli_protocol_fini(net_protocol_t protocol);

net_ws_cli_protocol_t net_ws_cli_protocol_create(
    net_schedule_t schedule,
    const char * protocol_name,
    /*protocol*/
    uint16_t protocol_capacity,
    net_ws_cli_protocol_init_fun_t protocol_init,
    net_ws_cli_protocol_fini_fun_t protocol_fini,
    /*endpoint*/
    uint16_t endpoint_capacity,
    net_ws_cli_endpoint_init_fun_t endpoint_init,
    net_ws_cli_endpoint_fini_fun_t endpoint_fini)
{
    net_protocol_t protocol =
        net_protocol_create(
            schedule,
            protocol_name,
            /*protocol*/
            sizeof(struct net_ws_cli_protocol) + protocol_capacity,
            net_ws_cli_protocol_init,
            net_ws_cli_protocol_fini,
            /*endpoint*/
            sizeof(struct net_ws_cli_endpoint) + endpoint_capacity,
            net_ws_cli_endpoint_init,
            net_ws_cli_endpoint_fini,
            net_ws_cli_endpoint_input,
            NULL,
            NULL,
            net_ws_cli_endpoint_on_state_change);
    if (protocol == NULL) {
        return NULL;
    }

    net_ws_cli_protocol_t ws_protocol = net_protocol_data(protocol);
    ws_protocol->m_protocol_capacity = protocol_capacity;
    ws_protocol->m_protocol_init = protocol_init;
    ws_protocol->m_protocol_fini = protocol_fini;
    ws_protocol->m_endpoint_capacity = endpoint_capacity;
    ws_protocol->m_endpoint_init = endpoint_init;
    ws_protocol->m_endpoint_fini = endpoint_fini;

    return ws_protocol;
}

void net_ws_cli_protocol_free(net_ws_cli_protocol_t ws_protocol) {
    net_protocol_free(net_protocol_from_data(ws_protocol));
}

static int net_ws_cli_protocol_init(net_protocol_t protocol) {
    net_ws_cli_protocol_t ws_protocol = net_protocol_data(protocol);

    ws_protocol->m_protocol_capacity = 0;
    ws_protocol->m_protocol_init = NULL;
    ws_protocol->m_protocol_fini = NULL;
    ws_protocol->m_endpoint_capacity = 0;
    ws_protocol->m_endpoint_init = NULL;
    ws_protocol->m_endpoint_fini = NULL;
    
    return 0;
}

static void net_ws_cli_protocol_fini(net_protocol_t protocol) {
}
