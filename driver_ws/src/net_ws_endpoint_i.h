#ifndef NET_WS_ENDPOINT_I_H_INCLEDED
#define NET_WS_ENDPOINT_I_H_INCLEDED
#include <wslay/wslay.h>
#include "cpe/utils/sha1.h"
#include "net_ws_endpoint.h"
#include "net_ws_protocol_i.h"

typedef enum net_ws_endpoint_handshake_cli_state {
    net_ws_endpoint_handshake_cli_first_line,
    net_ws_endpoint_handshake_cli_header_line,
} net_ws_endpoint_handshake_cli_state_t;

typedef enum net_ws_endpoint_handshake_cli_field {
    net_ws_endpoint_handshake_cli_field_upgrade,
    net_ws_endpoint_handshake_cli_field_connection,
    net_ws_endpoint_handshake_cli_field_accept,
} net_ws_endpoint_handshake_cli_field_t;

typedef enum net_ws_endpoint_handshake_svr_state {
    net_ws_endpoint_handshake_svr_first_line,
    net_ws_endpoint_handshake_svr_header_line,
} net_ws_endpoint_handshake_svr_state_t;

typedef enum net_ws_endpoint_handshake_svr_field {
    net_ws_endpoint_handshake_svr_field_upgrade,
    net_ws_endpoint_handshake_svr_field_connection,
    net_ws_endpoint_handshake_svr_field_key,
} net_ws_endpoint_handshake_svr_field_t;

struct net_ws_endpoint {
    net_endpoint_t m_base_endpoint;
    net_ws_stream_endpoint_t m_stream;

    void * m_ctx;
    net_ws_endpoint_on_msg_text_fun_t m_on_msg_text_fun;
    net_ws_endpoint_on_msg_bin_fun_t m_on_msg_bin_fun;
    net_ws_endpoint_on_close_fun_t m_on_close_fun;
    void (*m_ctx_free)(void*);

    net_ws_endpoint_runing_mode_t m_runing_mode;
    net_ws_endpoint_state_t m_state;
    union {
        struct {
            struct {
                char m_key[16];
                net_ws_endpoint_handshake_cli_state_t m_state;
                uint16_t m_readed_size;
                uint8_t m_received_fields;
            } m_handshake;
        } m_cli;
        struct {
            struct {
                net_ws_endpoint_handshake_svr_state_t m_state;
                uint16_t m_readed_size;
                uint8_t m_received_fields;
                struct cpe_sha1_value m_accept_key;
            } m_handshake;
        } m_svr;
    } m_state_data;

    uint8_t m_ws_ctx_is_processing;
    uint8_t m_ws_ctx_is_free;
    wslay_event_context_ptr m_ws_ctx;

    net_address_t m_host;
    char * m_path;
};

/**/
int net_ws_endpoint_init(net_endpoint_t base_endpoint);
void net_ws_endpoint_fini(net_endpoint_t base_endpoint);
int net_ws_endpoint_input(net_endpoint_t base_endpoint);
int net_ws_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

/**/
int net_ws_endpoint_set_state(net_ws_endpoint_t endpoint, net_ws_endpoint_state_t state);
void net_ws_endpoint_free_ws_ctx(net_ws_endpoint_t endpoint);

/*handshake*/
int net_ws_endpoint_send_handshake(net_endpoint_t base_endpoint, net_ws_endpoint_t endpoint);
int net_ws_endpoint_input_handshake_cli(net_endpoint_t base_endpoint, net_ws_endpoint_t endpoint);
int net_ws_endpoint_input_handshake_svr(net_endpoint_t base_endpoint, net_ws_endpoint_t endpoint);

#endif
