#ifndef NET_NGHTTP2_SESSION_H_I_INCLEDED
#define NET_NGHTTP2_SESSION_H_I_INCLEDED
#include "net_nghttp2_session.h"
#include "net_nghttp2_service_i.h"

struct net_nghttp2_session {
    TAILQ_ENTRY(net_nghttp2_session) m_next;
    nghttp2_session * m_session;
    net_nghttp2_request_list_t m_requests;
};

int net_nghttp2_session_init(net_endpoint_t endpoint);
void net_nghttp2_session_fini(net_endpoint_t endpoint);
int net_nghttp2_session_input(net_endpoint_t endpoint);

void net_nghttp2_session_timeout_reset(net_nghttp2_session_t session);
void net_nghttp2_session_schedule_close(net_nghttp2_session_t session);
void net_nghttp2_session_check_remove_done_requests(net_nghttp2_session_t session);

#endif
