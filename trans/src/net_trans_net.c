#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "net_timer.h"
#include "net_watcher.h"
#include "net_trans_manage_i.h"
#include "net_trans_task_i.h"

static void net_watcher_event_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
static void net_trans_check_multi_info(net_trans_manage_t mgr);

int net_trans_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp) {
    net_trans_manage_t mgr = cbp;
    net_trans_task_t task;

    curl_easy_getinfo(e, CURLINFO_PRIVATE, &task);

    assert(mgr);
    assert(task);

    if (mgr->m_debug >= 2) {
        static char * what_msgs[] = { "NONE", "IN", "OUT", "INOUT", "REMOVE" };
        CPE_INFO(mgr->m_em, "trans: %s-%d: expect %s, fd=%d!", mgr->m_name, task->m_id, what_msgs[what], s);
    }

    if (what == CURL_POLL_REMOVE) {
        if (task->m_watcher) {
            net_watcher_free(task->m_watcher);
            task->m_watcher = NULL;
        }
    }
    else {
        if (task->m_watcher == NULL) {
            task->m_watcher = net_watcher_create(mgr->m_driver, s, task, net_watcher_event_cb);
            if (task->m_watcher == NULL) {
                CPE_ERROR(mgr->m_em, "trans: sock_cb: %s-%d update watcher fail!", mgr->m_name, task->m_id);
                return -1;
            }
        }
        
        net_watcher_update(task->m_watcher, (what & CURL_POLL_IN ? 1 : 0), (what & CURL_POLL_OUT ? 1 : 0));
    }

    return 0;
}

void net_watcher_event_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_trans_task_t task = ctx;
    net_trans_manage_t mgr = task->m_mgr;
    int action;
    int rc;

    if (mgr->m_debug >= 2) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: action%s%s", mgr->m_name, task->m_id, (do_read ? " read" : ""), (do_write ? " write" : ""));
    }

    action = (do_read ? CURL_POLL_IN : 0) | (do_write ? CURL_POLL_OUT : 0);

    rc = curl_multi_socket_action(mgr->m_multi_handle, fd, action, &mgr->m_still_running);
    if (rc != CURLM_OK) {
        CPE_ERROR(mgr->m_em, "trans: event_cb: curl_multi_socket_action return error %d!", rc);
        net_trans_task_set_done(task, net_trans_result_cancel, -1, "curl_multi_socket_action");
        return;
    }

    net_trans_check_multi_info(mgr);

    if (mgr->m_still_running <= 0) {
        if(mgr->m_debug) {
            CPE_INFO(mgr->m_em, "trans: event_cb: last transfer done, kill timeout");
        }
        if (task->m_watcher) {
            net_watcher_update(task->m_watcher, 0, 0);
        }
    }
}

void net_trans_check_multi_info(net_trans_manage_t mgr) {
    CURLMsg *msg;
    int msgs_left;
    CURL * handler;
    net_trans_task_t task;

    while ((msg = curl_multi_info_read(mgr->m_multi_handle, &msgs_left))) {
        handler = msg->easy_handle;

        CURLcode rc = msg->data.result;
        
        task = NULL;
        curl_easy_getinfo(handler, CURLINFO_PRIVATE, &task);
        assert(task);
        if (task == NULL) continue;

        switch(msg->msg) {
        case CURLMSG_DONE: {
            if (rc == CURLE_OK) {
                net_trans_task_set_done(task, net_trans_result_complete, net_trans_task_error_none, NULL);
            }
            else {
                net_trans_task_error_t err = net_trans_task_error_internal;
                const char * addition_msg = NULL;
                switch (rc) {
                case CURLE_OPERATION_TIMEDOUT:
                    err = net_trans_task_error_timeout;
                    break;
                case CURLE_COULDNT_RESOLVE_HOST:
                    err = net_trans_task_error_dns_resolve_fail;
                    break;
                case CURLE_COULDNT_CONNECT: {
                    long os_error = 0;
                    if (curl_easy_getinfo(handler, CURLINFO_OS_ERRNO, &os_error) == CURLE_OK
                        && os_error != 0)
                    {
                        addition_msg = strerror((int)os_error);
                        switch(os_error) {
                        case ECONNRESET:
                            err = net_trans_task_error_remote_reset;
                            break;
                        case ETIMEDOUT:
                            err = net_trans_task_error_timeout;
                            break;
                        case ENETUNREACH:
                            err = net_trans_task_error_net_unreachable;
                            break;
                        case EHOSTUNREACH:
                            err = net_trans_task_error_host_unreachable;
                            break;
                        case ENETDOWN:
                            err = net_trans_task_error_net_down;
                            break;
                        default:
                            err = net_trans_task_error_connect;
                            break;
                        }
                    }
                    else {
                        err = net_trans_task_error_connect;
                    }
                    break;
                }
                default:
                    addition_msg = curl_easy_strerror(rc);
                    CPE_INFO(
                        mgr->m_em, "trans: %s-%d: check_multi_info: not processed CURLcode %d (%s)!",
                        mgr->m_name, task->m_id, rc, curl_easy_strerror(rc));
                    break;
                }

                net_trans_task_set_done(task, net_trans_result_error, err, addition_msg);
            }
            break;
        }
        default:
            CPE_INFO(mgr->m_em, "trans: %s-%d: check_multi_info recv msg %d!", mgr->m_name, task->m_id, msg->msg);
            break;
        }
    }
}

void net_trans_do_timeout(net_timer_t timer, void * ctx) {
    net_trans_manage_t mgr = ctx;
    CURLMcode rc;

    rc = curl_multi_socket_action(mgr->m_multi_handle, CURL_SOCKET_TIMEOUT, 0, &mgr->m_still_running);
    if (rc != CURLM_OK) {
        CPE_ERROR(mgr->m_em, "trans: do_timer: curl_multi_socket_action return error %d!", rc);
    }

    net_trans_check_multi_info(mgr);
}

int net_trans_timer_cb(CURLM *multi, long timeout_ms, net_trans_manage_t mgr) {
    if (mgr->m_debug >= 2) {
        CPE_INFO(mgr->m_em, "trans: timer_cb: timeout_ms=%d", (int)timeout_ms);
    }

    net_timer_active(mgr->m_timer_event, (int32_t)timeout_ms);
    return 0;
}
