#ifndef NET_SSL_TYPES_H_INCLEDED
#define NET_SSL_TYPES_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

enum net_ssl_endpoint_runing_mode {
    net_ssl_endpoint_runing_mode_init,
    net_ssl_endpoint_runing_mode_cli,
    net_ssl_endpoint_runing_mode_svr,
};

typedef enum net_ssl_endpoint_runing_mode net_ssl_endpoint_runing_mode_t;
typedef enum net_ssl_endpoint_state net_ssl_endpoint_state_t;

typedef struct net_ssl_protocol * net_ssl_protocol_t;
typedef struct net_ssl_endpoint * net_ssl_endpoint_t;

typedef struct net_ssl_stream_driver * net_ssl_stream_driver_t;
typedef struct net_ssl_stream_endpoint * net_ssl_stream_endpoint_t;
typedef struct net_ssl_stream_acceptor * net_ssl_stream_acceptor_t;

NET_END_DECL

#endif
