#ifndef NET_WS_SVR_ENDPOINT_I_H_INCLEDED
#define NET_WS_SVR_ENDPOINT_I_H_INCLEDED
#include "cpe/utils/sha1.h"
#include "net_ws_svr_endpoint.h"
#include "net_ws_svr_protocol_i.h"

typedef enum net_ws_svr_endpoint_handshake_state {
    net_ws_svr_endpoint_handshake_first_line,
    net_ws_svr_endpoint_handshake_header_line,
} net_ws_svr_endpoint_handshake_state_t;

typedef enum net_ws_svr_endpoint_handshake_field {
    net_ws_svr_endpoint_handshake_field_upgrade,
    net_ws_svr_endpoint_handshake_field_connection,
    net_ws_svr_endpoint_handshake_field_key,
} net_ws_svr_endpoint_handshake_field_t;

struct net_ws_svr_endpoint {
    net_endpoint_t m_base_endpoint;
    net_ws_svr_stream_endpoint_t m_stream;
    net_ws_svr_endpoint_state_t m_state;
    union {
        struct {
            net_ws_svr_endpoint_handshake_state_t m_state;
            uint16_t m_readed_size;
            uint8_t m_received_fields;
            struct cpe_sha1_value m_accept_key;
        } m_handshake;
    };
    net_address_t m_host;
    char * m_path;
    uint8_t m_version;
};

int net_ws_svr_endpoint_set_path(net_ws_svr_endpoint_t endpoint, const char * path);
int net_ws_svr_endpoint_set_host(net_ws_svr_endpoint_t endpoint, net_address_t address);

/**/
int net_ws_svr_endpoint_write(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf);

/*protocol*/
int net_ws_svr_endpoint_init(net_endpoint_t base_endpoint);
void net_ws_svr_endpoint_fini(net_endpoint_t base_endpoint);
int net_ws_svr_endpoint_input(net_endpoint_t base_endpoint);
int net_ws_svr_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

/*handshake*/
int net_ws_svr_endpoint_input_handshake(net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint);

#endif
