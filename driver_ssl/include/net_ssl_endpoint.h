#ifndef NET_SSL_ENDPOINT_H_INCLEDED
#define NET_SSL_ENDPOINT_H_INCLEDED
#include "net_ssl_types.h"

NET_BEGIN_DECL

enum net_ssl_endpoint_state {
    net_ssl_endpoint_state_init,
    net_ssl_endpoint_state_handshake,
    net_ssl_endpoint_state_streaming,
};

typedef void (*net_ssl_endpoint_on_msg_text_fun_t)(void * ctx, net_ssl_endpoint_t endpoin, const char * msg);
typedef void (*net_ssl_endpoint_on_msg_bin_fun_t)(void * ctx, net_ssl_endpoint_t endpoin, const void * msg, uint32_t msg_len);

net_ssl_endpoint_t net_ssl_endpoint_cast(net_endpoint_t endpoint);
net_ssl_stream_endpoint_t net_ssl_endpoint_stream(net_ssl_endpoint_t endpoint);

void net_ssl_endpoint_free(net_ssl_endpoint_t endpoint);
    
net_ssl_endpoint_state_t net_ssl_endpoint_state(net_ssl_endpoint_t endpoint);
net_ssl_endpoint_runing_mode_t net_ssl_endpoint_runing_mode(net_ssl_endpoint_t endpoint);
int net_ssl_endpoint_set_runing_mode(net_ssl_endpoint_t endpoint, net_ssl_endpoint_runing_mode_t runing_mode);
net_endpoint_t net_ssl_endpoint_base_endpoint(net_ssl_endpoint_t endpoint);

const char * net_ssl_endpoint_state_str(net_ssl_endpoint_state_t state);
const char * net_ssl_endpoint_runing_mode_str(net_ssl_endpoint_runing_mode_t runing_mode);

NET_END_DECL

#endif

