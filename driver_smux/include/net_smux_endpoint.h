#ifndef NET_SMUX_ENDPOINT_H_INCLEDED
#define NET_SMUX_ENDPOINT_H_INCLEDED
#include "net_smux_types.h"

NET_BEGIN_DECL

enum net_smux_endpoint_runing_mode {
    net_smux_endpoint_runing_mode_init,
    net_smux_endpoint_runing_mode_cli,
    net_smux_endpoint_runing_mode_svr,
};

net_smux_endpoint_t net_smux_endpoint_cast(net_endpoint_t endpoint);

void net_smux_endpoint_free(net_smux_endpoint_t endpoint);

net_smux_endpoint_runing_mode_t net_smux_endpoint_runing_mode(net_smux_endpoint_t endpoint);
int net_smux_endpoint_set_runing_mode(net_smux_endpoint_t endpoint, net_smux_endpoint_runing_mode_t runing_mode);

net_endpoint_t net_smux_endpoint_base_endpoint(net_smux_endpoint_t endpoint);
net_smux_session_t net_smux_endpoint_session(net_smux_endpoint_t endpoint);

/*utils*/
const char * net_smux_endpoint_runing_mode_str(net_smux_endpoint_runing_mode_t runing_mode);

NET_END_DECL

#endif
