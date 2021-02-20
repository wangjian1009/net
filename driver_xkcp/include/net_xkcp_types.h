#ifndef NET_XKCP_TYPES_H_INCLEDED
#define NET_XKCP_TYPES_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_xkcp_mode net_xkcp_mode_t;
typedef enum net_xkcp_endpoint_runing_mode net_xkcp_endpoint_runing_mode_t;

typedef struct net_xkcp_config * net_xkcp_config_t;
typedef struct net_xkcp_driver * net_xkcp_driver_t;
typedef struct net_xkcp_endpoint * net_xkcp_endpoint_t;
typedef struct net_xkcp_acceptor * net_xkcp_acceptor_t;
typedef struct net_xkcp_connector * net_xkcp_connector_t;

NET_END_DECL

#endif
