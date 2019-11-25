#ifndef NET_HTTP2_SVR_STREAM_H_I_INCLEDED
#define NET_HTTP2_SVR_STREAM_H_I_INCLEDED
#include "net_http2_svr_stream.h"
#include "net_http2_svr_service_i.h"

struct net_http2_svr_stream {
    TAILQ_ENTRY(net_http2_svr_stream) m_next;
    net_timer_t m_timer_timeout;
    net_timer_t m_timer_close;
    //net_http2_svr_after_write_cb after_write_cb; /* ro */
    net_http2_svr_request_list_t m_requests;
};

int net_http2_svr_stream_init(net_endpoint_t endpoint);
void net_http2_svr_stream_fini(net_endpoint_t endpoint);
int net_http2_svr_stream_input(net_endpoint_t endpoint);

void net_http2_svr_stream_timeout_reset(net_http2_svr_stream_t stream);
void net_http2_svr_stream_schedule_close(net_http2_svr_stream_t stream);
void net_http2_svr_stream_check_remove_done_requests(net_http2_svr_stream_t stream);

#endif
