#ifndef NET_HTTP_SVR_CONNECTION_H_I_INCLEDED
#define NET_HTTP_SVR_CONNECTION_H_I_INCLEDED
#include "net_http_svr_endpoint.h"
#include "net_http_svr_protocol_i.h"
#include "net_http_svr_request_parser.h"

struct net_http_svr_endpoint {
    TAILQ_ENTRY(net_http_svr_endpoint) m_next;
    net_timer_t m_timer_timeout;
    net_timer_t m_timer_close;
    //net_http_svr_after_write_cb after_write_cb; /* ro */
    struct net_http_svr_request_parser parser; /* private */
    net_http_svr_request_list_t m_requests;
};

int net_http_svr_endpoint_init(net_endpoint_t endpoint);
void net_http_svr_endpoint_fini(net_endpoint_t endpoint);
int net_http_svr_endpoint_input(net_endpoint_t endpoint);

void net_http_svr_endpoint_timeout_reset(net_endpoint_t endpoint);
void net_http_svr_endpoint_schedule_close(net_endpoint_t endpoint);
void net_http_svr_endpoint_check_remove_done_requests(net_http_svr_endpoint_t connection);

#endif
