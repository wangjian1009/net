#include "assert.h"
#include "net_protocol.h"
#include "net_trans_protocol_i.h"
#include "net_trans_endpoint_i.h"
#include "net_trans_task_i.h"

static int net_trans_protocol_init(net_protocol_t protocol);
static void net_trans_protocol_fini(net_protocol_t protocol);
static int net_trans_curl_timer_cb(CURLM *multi, long timeout_ms, net_trans_protocol_t trans_protocol);

net_trans_protocol_t net_trans_protocol_create(net_trans_manage_t manage) {
    net_protocol_t protocol = net_protocol_find(manage->m_schedule, "trans-http");
    if (protocol == NULL) {
        protocol =
            net_protocol_create(
                manage->m_schedule,
                "trans-http",
                /*protocol*/
                sizeof(struct net_trans_protocol),
                net_trans_protocol_init,
                net_trans_protocol_fini,
                /*endpoint*/
                sizeof(struct net_trans_endpoint),
                net_trans_endpoint_init,
                net_trans_endpoint_fini,
                net_trans_endpoint_input,
                NULL/*forward*/,
                NULL,
                net_trans_endpoint_on_state_change);
        if (protocol == NULL) {
            return NULL;
        }
    }
    
    net_trans_protocol_t trans_protocol = net_protocol_data(protocol);
    trans_protocol->m_ref_count++;
    return trans_protocol;
}

void net_trans_protocol_free(net_trans_protocol_t trans_protocol) {
    assert(trans_protocol->m_ref_count > 0);
    trans_protocol->m_ref_count--;
    if (trans_protocol->m_ref_count == 0) {
        net_protocol_free(net_protocol_from_data(trans_protocol));
    }
}

static int net_trans_protocol_init(net_protocol_t protocol) {
    net_trans_protocol_t trans_protocol = net_protocol_data(protocol);
    
    trans_protocol->m_ref_count = 0;

	trans_protocol->m_multi_handle = curl_multi_init();
	if (trans_protocol->m_multi_handle == NULL) return -1;

    curl_multi_setopt(trans_protocol->m_multi_handle, CURLMOPT_SOCKETFUNCTION, net_trans_curl_sock_cb);
    curl_multi_setopt(trans_protocol->m_multi_handle, CURLMOPT_SOCKETDATA, trans_protocol);
    curl_multi_setopt(trans_protocol->m_multi_handle, CURLMOPT_TIMERFUNCTION, net_trans_curl_timer_cb);
    curl_multi_setopt(trans_protocol->m_multi_handle, CURLMOPT_TIMERDATA, trans_protocol);

    return 0;
}

static void net_trans_protocol_fini(net_protocol_t protocol) {
    net_trans_protocol_t trans_protocol = net_protocol_data(protocol);

    if (trans_protocol->m_multi_handle) {
        curl_multi_cleanup(trans_protocol->m_multi_handle);
        trans_protocol->m_multi_handle = NULL;
    }
}

static int net_trans_protocol_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp) {
    net_trans_protocol_t trans_protocol = cbp;

    net_trans_task_t task;
    curl_easy_getinfo(e, CURLINFO_PRIVATE, &task);
    assert(task);

    net_trans_manage_t mgr = task->m_mgr;
    assert(mgr);

    if (mgr->m_debug >= 2) {
        static char * what_msgs[] = { "NONE", "IN", "OUT", "INOUT", "REMOVE" };
        CPE_INFO(
            mgr->m_em, "net_trans: sock_cb: task %d %s!",
            task->m_id, what_msgs[what]);
    }

    if (what == CURL_POLL_REMOVE) {
        /* task->m_sockfd = -1; */
        /* if (task->m_evset) { */
        /*     ev_io_stop(mgr->m_loop, &task->m_watch); */
        /*     task->m_evset = 0; */
        /* } */
    }
    else {
        /* int kind = */
        /*     (what & CURL_POLL_IN ? EV_READ : 0) */
        /*     | (what & CURL_POLL_OUT ? EV_WRITE : 0); */

        /* task->m_sockfd = s; */
        /* if (task->m_evset) { */
        /*     ev_io_stop(mgr->m_loop, &task->m_watch); */
        /* } */
        /* task->m_evset = 1; */
    }

    return 0;
}

static int net_trans_curl_timer_cb(CURLM *multi, long timeout_ms, net_trans_protocol_t trans_protocol) {
    return 0;
}
