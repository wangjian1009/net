#ifndef NET_SSL_ENDPOINT_I_H_INCLEDED
#define NET_SSL_ENDPOINT_I_H_INCLEDED
#include "mbedtls/timing.h"
#include "net_ssl_endpoint.h"
#include "net_ssl_protocol_i.h"

struct net_ssl_endpoint {
    net_endpoint_t m_base_endpoint;
    net_ssl_stream_endpoint_t m_stream;
    net_ssl_endpoint_runing_mode_t m_runing_mode;
    net_ssl_endpoint_state_t m_state;
    mbedtls_ssl_config * m_ssl_config;
	mbedtls_ssl_context * m_ssl;
    mbedtls_timing_delay_context m_timer;
};

int net_ssl_endpoint_init(net_endpoint_t base_endpoint);
void net_ssl_endpoint_fini(net_endpoint_t base_endpoint);
int net_ssl_endpoint_input(net_endpoint_t base_endpoint);
int net_ssl_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);
void net_ssl_endpoint_calc_size(net_endpoint_t base_endpoint, net_endpoint_size_info_t size_info);

int net_ssl_endpoint_write(
    net_endpoint_t base_underline, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf);

int net_ssl_endpoint_set_runing_mode(net_ssl_endpoint_t endpoint, net_ssl_endpoint_runing_mode_t runing_mode);
int net_ssl_endpoint_set_state(net_ssl_endpoint_t endpoint, net_ssl_endpoint_state_t state);

#endif
