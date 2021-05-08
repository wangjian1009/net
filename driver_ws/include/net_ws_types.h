#ifndef NET_WS_TYPES_H_INCLEDED
#define NET_WS_TYPES_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_ws_endpoint_runing_mode net_ws_endpoint_runing_mode_t;
typedef enum net_ws_endpoint_state net_ws_endpoint_state_t;

typedef struct net_ws_protocol * net_ws_protocol_t;
typedef struct net_ws_endpoint * net_ws_endpoint_t;
typedef struct net_ws_stream_driver * net_ws_stream_driver_t;
typedef struct net_ws_stream_endpoint * net_ws_stream_endpoint_t;
typedef struct net_ws_stream_acceptor * net_ws_stream_acceptor_t;

/*
 * Status codes defined in RFC6455
 */
enum net_ws_status_code {
    net_ws_status_code_normal_closure = 1000,
    net_ws_status_code_going_away = 1001,
    net_ws_status_code_protocol_error = 1002,
    net_ws_status_code_unsupported_data = 1003,
    net_ws_status_code_no_status_rcvd = 1005,
    net_ws_status_code_abnormal_closure = 1006,
    net_ws_status_code_invalid_frame_payload_data = 1007,
    net_ws_status_code_policy_violation = 1008,
    net_ws_status_code_message_too_big = 1009,
    net_ws_status_code_mandatory_ext = 1010,
    net_ws_status_code_internal_server_error = 1011,
    net_ws_status_code_tls_handshake = 1015
};

NET_END_DECL

#endif
