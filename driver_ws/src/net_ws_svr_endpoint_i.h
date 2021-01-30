#ifndef NET_WS_SVR_ENDPOINT_I_H_INCLEDED
#define NET_WS_SVR_ENDPOINT_I_H_INCLEDED
#include "net_ws_svr_endpoint.h"
#include "net_ws_svr_protocol_i.h"

/*endpoint*/
enum net_ws_svr_endpoint_ws_state {
    net_ws_svr_endpoint_ws_handshake,
    net_ws_svr_endpoint_ws_open,
};

struct net_ws_svr_endpoint {
    net_ws_svr_stream_endpoint_t m_stream;
    enum net_ws_svr_endpoint_ws_state m_state;
};

int net_ws_svr_endpoint_write(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf);

int net_ws_svr_endpoint_init(net_endpoint_t base_endpoint);
void net_ws_svr_endpoint_fini(net_endpoint_t base_endpoint);
int net_ws_svr_endpoint_input(net_endpoint_t base_endpoint);
int net_ws_svr_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

#endif
