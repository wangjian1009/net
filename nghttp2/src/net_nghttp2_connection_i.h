#ifndef NET_NGHTTP2_CONNECTION_H_I_INCLEDED
#define NET_NGHTTP2_CONNECTION_H_I_INCLEDED
#include "net_nghttp2_connection.h"
#include "net_nghttp2_service_i.h"

struct net_nghttp2_connection {
    TAILQ_ENTRY(net_nghttp2_connection) m_next;
    net_timer_t m_timer_timeout;
    net_timer_t m_timer_close;
    //net_nghttp2_after_write_cb after_write_cb; /* ro */
    net_nghttp2_request_list_t m_requests;
};

int net_nghttp2_connection_init(net_endpoint_t endpoint);
void net_nghttp2_connection_fini(net_endpoint_t endpoint);
int net_nghttp2_connection_input(net_endpoint_t endpoint);

void net_nghttp2_connection_timeout_reset(net_nghttp2_connection_t connection);
void net_nghttp2_connection_schedule_close(net_nghttp2_connection_t connection);
void net_nghttp2_connection_check_remove_done_requests(net_nghttp2_connection_t connection);

#endif
