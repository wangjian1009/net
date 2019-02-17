#include "assert.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_strings.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_trans_endpoint_i.h"
#include "net_trans_task_i.h"

static void net_trans_endpoint_on_data_evt(
    void * ctx, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, net_endpoint_data_event_t evt, uint32_t size);

void net_trans_endpoint_free(net_trans_endpoint_t trans) {
    net_endpoint_free(net_endpoint_from_data(trans));
}

int net_trans_endpoint_init(net_endpoint_t endpoint) {
    net_trans_endpoint_t trans = net_endpoint_data(endpoint);
    trans->m_mgr = NULL;
    trans->m_task_count = 0;
    TAILQ_INIT(&trans->m_tasks);

    return 0;
}

void net_trans_endpoint_fini(net_endpoint_t endpoint) {
    net_trans_endpoint_t trans = net_endpoint_data(endpoint);

    while(!TAILQ_EMPTY(&trans->m_tasks)) {
        net_trans_task_free(TAILQ_FIRST(&trans->m_tasks));
    }
    assert(trans->m_task_count == 0);

    assert(trans->m_mgr->m_endpoint_count > 0);
    trans->m_mgr->m_endpoint_count--;
    TAILQ_REMOVE(&trans->m_mgr->m_endpoints, trans, m_next);
    trans->m_mgr = NULL;
}

int net_trans_endpoint_input(net_endpoint_t endpoint) {
    return 0;
}

int net_trans_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state) {
    /* switch(net_endpoint_state(endpoint)) { */
    /* case net_http_state_disable: */
    /*     return -1; */
    /* case net_http_state_error: */
    /*     return -1; */
    /* case net_http_state_connecting: */
    /* case net_http_state_established: */
    /*     return 0; */
    /* } */
    return 0;
}

uint8_t net_trans_endpoint_is_https(net_trans_endpoint_t trans) {
    return 0;
//    return net_endpoint_use_https(net_endpoint_from_data(trans)) ? 1 : 0;
}

/* uint8_t net_trans_endpoint_is_active(net_trans_endpoint_t trans) { */
/*     switch(net_endpoint_state(net_endpoint_from_data(trans))) { */
/*     case net_http_state_disable: */
/*     case net_http_state_error: */
/*         return 0; */
/*     default: */
/*         return 1; */
/*     } */
/* } */

static void net_trans_endpoint_on_data_evt(
    void * ctx, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, net_endpoint_data_event_t evt, uint32_t size)
{
    net_trans_endpoint_t trans = ctx;

    if (trans->m_mgr->m_watcher_fun) {
        trans->m_mgr->m_watcher_fun(trans->m_mgr->m_watcher_ctx, endpoint, buf_type, evt, size);
    }
}

curl_socket_t net_trans_curl_opensocket_cb(void *clientp, curlsocktype purpose, struct curl_sockaddr *address) {
    net_trans_task_t task = clientp;
    net_trans_manage_t mgr = task->m_mgr;

    assert(task->m_ep == NULL);
    
    net_endpoint_t base_endpoint =
        net_endpoint_create(mgr->m_driver, net_protocol_from_data(mgr->m_protocol));
    if (base_endpoint == NULL) {
        CPE_ERROR(mgr->m_em, "trans: create http-endpoint fail!");
        return CURL_SOCKET_BAD;
    }

    net_trans_endpoint_t ep = net_endpoint_data(base_endpoint);
    ep->m_mgr = mgr;
    TAILQ_INSERT_TAIL(&mgr->m_endpoints, ep, m_next);

    net_endpoint_set_data_watcher(base_endpoint, ep, net_trans_endpoint_on_data_evt, NULL);

    task->m_ep = ep;
    TAILQ_INSERT_TAIL(&ep->m_tasks, task, m_next_for_ep);
    
    return (curl_socket_t)net_endpoint_id(base_endpoint);
}

int net_trans_curl_closesocket_cb(void *clientp, curl_socket_t item) {
    net_trans_task_t task = clientp;
    net_trans_manage_t mgr = task->m_mgr;

    net_endpoint_t base_endpoint = net_endpoint_find(mgr->m_schedule, (uint32_t)item);
    if (base_endpoint) {
        net_endpoint_free(base_endpoint);
    }

    return 0;
}

int net_trans_curl_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp) {
    return 0;
}
