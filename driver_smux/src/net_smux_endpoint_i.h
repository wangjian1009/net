#ifndef NET_SMUX_ENDPOINT_I_H_INCLEDED
#define NET_SMUX_ENDPOINT_I_H_INCLEDED
#include "net_smux_endpoint.h"
#include "net_smux_protocol_i.h"

struct net_smux_endpoint {
    net_endpoint_t m_base_endpoint;
    net_smux_endpoint_runing_mode_t m_runing_mode;
    net_smux_session_t m_session;
};

/*protocol*/
int net_smux_endpoint_init(net_endpoint_t base_endpoint);
void net_smux_endpoint_fini(net_endpoint_t base_endpoint);
int net_smux_endpoint_input(net_endpoint_t base_endpoint);
int net_smux_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

void net_smux_session_set_session(net_smux_endpoint_t endpoit, net_smux_session_t session);

#endif
