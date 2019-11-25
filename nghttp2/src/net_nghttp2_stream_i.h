#ifndef NET_NGHTTP2_STREAM_H_I_INCLEDED
#define NET_NGHTTP2_STREAM_H_I_INCLEDED
#include "net_nghttp2_stream.h"
#include "net_nghttp2_service_i.h"

struct net_nghttp2_stream {
    TAILQ_ENTRY(net_nghttp2_stream) m_next;
    net_timer_t m_timer_timeout;
    net_timer_t m_timer_close;
    //net_nghttp2_after_write_cb after_write_cb; /* ro */
    net_nghttp2_request_list_t m_requests;
};

int net_nghttp2_stream_init(net_endpoint_t endpoint);
void net_nghttp2_stream_fini(net_endpoint_t endpoint);
int net_nghttp2_stream_input(net_endpoint_t endpoint);

void net_nghttp2_stream_timeout_reset(net_nghttp2_stream_t stream);
void net_nghttp2_stream_schedule_close(net_nghttp2_stream_t stream);
void net_nghttp2_stream_check_remove_done_requests(net_nghttp2_stream_t stream);

#endif
