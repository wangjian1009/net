#ifndef NET_HTTP2_ENDPOINT_I_H_INCLEDED
#define NET_HTTP2_ENDPOINT_I_H_INCLEDED
#include "net_http2_endpoint.h"
#include "net_http2_protocol_i.h"

struct net_http2_endpoint {
    net_endpoint_t m_base_endpoint;
    net_http2_endpoint_runing_mode_t m_runing_mode;
    net_http2_endpoint_state_t m_state;
    struct {
        uint16_t m_stream_capacity;
    } m_remote_settings;
    nghttp2_session * m_http2_session;
    union {
        struct {
            net_http2_stream_group_t m_stream_group;
            TAILQ_ENTRY(net_http2_endpoint) m_next_for_group;
        } m_cli;
        struct {
            net_http2_stream_acceptor_t m_stream_acceptor;
            TAILQ_ENTRY(net_http2_endpoint) m_next_for_acceptor;
        } m_svr;
    };
    net_http2_stream_endpoint_list_t m_streams;
    net_http2_stream_endpoint_list_t m_sending_streams;
    uint8_t m_in_processing;
    net_timer_t m_delay_processor;
};

/*protocol*/
int net_http2_endpoint_init(net_endpoint_t base_endpoint);
void net_http2_endpoint_fini(net_endpoint_t base_endpoint);
int net_http2_endpoint_input(net_endpoint_t base_endpoint);
int net_http2_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

/**/
int net_http2_endpoint_set_state(net_http2_endpoint_t endpoint, net_http2_endpoint_state_t state);
void net_http2_endpoint_schedule_delay_processor(net_http2_endpoint_t endpoint);
void net_http2_endpoint_schedule_flush(net_http2_endpoint_t endpoint);

/*cli*/
void net_http2_endpoint_set_stream_group(
    net_http2_endpoint_t endpoint, net_http2_stream_group_t stream_group);

/*svr*/
void net_http2_endpoint_set_stream_acceptor(
    net_http2_endpoint_t endpoint, net_http2_stream_acceptor_t stream_acceptor);

int net_http2_endpoint_http2_send_settings(net_http2_endpoint_t endpoint);

/*nghttp2*/
ssize_t net_http2_endpoint_send_callback(
    nghttp2_session * session, const uint8_t * data, size_t length, int flags, void * user_data);

int net_http2_endpoint_on_frame_recv_callback(
    nghttp2_session * session, const nghttp2_frame * frame, void * user_data);

int net_http2_endpoint_on_frame_send_callback(
    nghttp2_session * session, const nghttp2_frame * frame, void * user_data);

int net_http2_endpoint_on_frame_not_send_callback(
    nghttp2_session * session, const nghttp2_frame * frame, int lib_error_code,  void * user_data);

int net_http2_endpoint_on_data_chunk_recv_callback(
    nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data);

int net_http2_endpoint_on_stream_close_callback(
    nghttp2_session * session, int32_t stream_id, uint32_t error_code, void * user_data);

int net_http2_endpoint_on_header_callback(
    nghttp2_session *session,
    const nghttp2_frame *frame, const uint8_t *name,
    size_t namelen, const uint8_t *value,
    size_t valuelen, uint8_t flags, void *user_data);

int net_http2_endpoint_on_invalid_header_callback(
    nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name,
    size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags,
    void *user_data);

int net_http2_endpoint_on_begin_headers_callback(
    nghttp2_session *session, const nghttp2_frame *frame, void *user_data);

int net_http2_endpoint_on_error_callback(
    nghttp2_session *session,
    int lib_error_code, const char *msg, size_t len, void *user_data);

int net_http2_endpoint_send_data_callback(
    nghttp2_session *session,
    nghttp2_frame *frame, const uint8_t *framehd, size_t length, nghttp2_data_source *source, void *user_data);

#endif
