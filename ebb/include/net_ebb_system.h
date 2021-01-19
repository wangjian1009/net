#ifndef NET_EBB_SYSTEM_H
#define NET_EBB_SYSTEM_H
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_ebb_protocol * net_ebb_protocol_t;
typedef struct net_ebb_connection * net_ebb_connection_t;
typedef struct net_ebb_request * net_ebb_request_t;
typedef struct net_ebb_request_header * net_ebb_request_header_t;
typedef struct net_ebb_request_header_it * net_ebb_request_header_it_t;
typedef struct net_ebb_response * net_ebb_response_t;
typedef struct net_ebb_mount_point * net_ebb_mount_point_t;
typedef struct net_ebb_processor * net_ebb_processor_t;

typedef enum net_ebb_request_method net_ebb_request_method_t;
typedef enum net_ebb_request_transfer_encoding net_ebb_request_transfer_encoding_t;
typedef enum net_ebb_request_state net_ebb_request_state_t;
typedef enum net_ebb_response_state net_ebb_response_state_t;

NET_END_DECL

#endif
