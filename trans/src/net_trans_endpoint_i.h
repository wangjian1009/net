#ifndef NET_TRANS_HTTP_ENDPOINT_I_H_INCLEDED
#define NET_TRANS_HTTP_ENDPOINT_I_H_INCLEDED
#include "net_trans_manage_i.h"

NET_BEGIN_DECL

struct net_trans_endpoint {
    net_trans_manage_t m_mgr;
    TAILQ_ENTRY(net_trans_endpoint) m_next;
    uint16_t m_task_count;
    net_trans_task_list_t m_tasks;
};

void net_trans_endpoint_free(net_trans_endpoint_t trans);

uint8_t net_trans_endpoint_is_https(net_trans_endpoint_t trans);
uint8_t net_trans_endpoint_is_active(net_trans_endpoint_t trans);

int net_trans_endpoint_init(net_endpoint_t endpoint);
void net_trans_endpoint_fini(net_endpoint_t endpoint);
int net_trans_endpoint_input(net_endpoint_t endpoint);

int net_trans_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state);

curl_socket_t net_trans_curl_opensocket_cb(void *clientp, curlsocktype purpose, struct curl_sockaddr *address);
int net_trans_curl_closesocket_cb(void *clientp, curl_socket_t item);
int net_trans_curl_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp);

NET_END_DECL

#endif
