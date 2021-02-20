#ifndef NET_XKCP_ENDPOINT_I_H_INCLEDED
#define NET_XKCP_ENDPOINT_I_H_INCLEDED
#include "net_xkcp_endpoint.h"
#include "net_xkcp_driver_i.h"

struct net_xkcp_endpoint {
    net_endpoint_t m_base_endpoint;
    ikcpcb * m_kcp;
    net_xkcp_endpoint_runing_mode_t m_runing_mode;
    union {
        struct {
            net_xkcp_connector_t m_connector;
            cpe_hash_entry m_hh_for_connector;
        } m_cli;
        struct {
            net_xkcp_client_t m_client;
            cpe_hash_entry m_hh_for_client;
        } m_svr;
    };
};

int net_xkcp_endpoint_init(net_endpoint_t base_endpoint);
void net_xkcp_endpoint_fini(net_endpoint_t base_endpoint);
int net_xkcp_endpoint_connect(net_endpoint_t base_endpoint);
int net_xkcp_endpoint_update(net_endpoint_t base_endpoint);
int net_xkcp_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay);
int net_xkcp_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss);

void net_xkcp_endpoint_set_running_mode(net_xkcp_endpoint_t endpoint, net_xkcp_endpoint_runing_mode_t runing_mode);
int net_xkcp_endpoint_set_conv(net_xkcp_endpoint_t endpoint, uint32_t conv);

int net_xkcp_endpoint_set_connector(net_xkcp_endpoint_t endpoint, net_xkcp_connector_t connector);

int net_xkcp_endpoint_set_client(net_xkcp_endpoint_t endpoint, net_xkcp_client_t client);

int net_xkcp_endpoint_eq(net_xkcp_endpoint_t l, net_xkcp_endpoint_t r, void * user_data);
uint32_t net_xkcp_endpoint_hash(net_xkcp_endpoint_t o, void * user_data);

const char * net_xkcp_endpoint_runing_mode_str(net_xkcp_endpoint_runing_mode_t runing_mode);

#endif
