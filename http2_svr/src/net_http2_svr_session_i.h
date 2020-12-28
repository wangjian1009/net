#ifndef NET_HTTP2_SVR_SESSION_H_I_INCLEDED
#define NET_HTTP2_SVR_SESSION_H_I_INCLEDED
#include "net_http2_svr_session.h"
#include "net_http2_svr_service_i.h"

struct net_http2_svr_session {
    TAILQ_ENTRY(net_http2_svr_session) m_next;
    nghttp2_session * m_session;
    net_http2_svr_request_list_t m_requests;
};

int net_http2_svr_session_init(net_endpoint_t endpoint);
void net_http2_svr_session_fini(net_endpoint_t endpoint);
int net_http2_svr_session_input(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);

void net_http2_svr_session_timeout_reset(net_http2_svr_session_t session);
void net_http2_svr_session_schedule_close(net_http2_svr_session_t session);
void net_http2_svr_session_check_remove_done_requests(net_http2_svr_session_t session);

#endif
