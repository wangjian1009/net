#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "net_timer.h"
#include "net_watcher.h"
#include "net_log_request_manage.h"
#include "net_log_request.h"
#include "net_log_category_i.h"

static void net_watcher_event_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
static void net_log_request_manage_check_multi_info(net_log_schedule_t schedule, net_log_request_manage_t mgr);

int net_log_request_manage_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp) {
    net_log_request_manage_t mgr = cbp;
    net_log_schedule_t schedule = mgr->m_schedule;
    net_log_request_t request;

    curl_easy_getinfo(e, CURLINFO_PRIVATE, &request);

    assert(mgr);
    assert(request);
    
    net_log_category_t category = request->m_category;

    if (schedule->m_debug >= 2) {
        static char * what_msgs[] = { "NONE", "IN", "OUT", "INOUT", "REMOVE" };
        CPE_INFO(schedule->m_em, "log: category [%d]%s: sock_cb: expect %s, fd=%d!", category->m_id, category->m_name, what_msgs[what], s);
    }

    if (what == CURL_POLL_REMOVE) {
        if (request->m_watcher) {
            net_watcher_free(request->m_watcher);
            request->m_watcher = NULL;
        }
    }
    else {
        if (request->m_watcher == NULL) {
            request->m_watcher = net_watcher_create(mgr->m_net_driver, s, request, net_watcher_event_cb);
            if (request->m_watcher == NULL) {
                CPE_ERROR(
                    schedule->m_em, "log: category [%d]%s: sock_cb: update watcher fail!",
                    category->m_id, category->m_name);
                return -1;
            }
        }
        
        net_watcher_update(request->m_watcher, (what & CURL_POLL_IN ? 1 : 0), (what & CURL_POLL_OUT ? 1 : 0));
    }

    return 0;
}

void net_watcher_event_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_log_request_t request = ctx;
    net_log_category_t category = request->m_category;
    net_log_request_manage_t mgr = request->m_mgr;
    net_log_schedule_t schedule = mgr->m_schedule;
    int action;
    int rc;

    if (schedule->m_debug >= 2) {
        CPE_INFO(
            schedule->m_em, "log: category [%d]%s: action%s%s",
            category->m_id, category->m_name, (do_read ? " read" : ""), (do_write ? " write" : ""));
    }

    action = (do_read ? CURL_POLL_IN : 0) | (do_write ? CURL_POLL_OUT : 0);

    rc = curl_multi_socket_action(mgr->m_multi_handle, fd, action, &mgr->m_still_running);
    if (rc != CURLM_OK) {
        CPE_ERROR(schedule->m_em, "log: event_cb: curl_multi_socket_action return error %d!", rc);
        net_log_request_complete(request, net_log_request_complete_cancel);
        return;
    }

    net_log_request_manage_check_multi_info(schedule, mgr);

    if (mgr->m_still_running <= 0) {
        if(schedule->m_debug) {
            CPE_INFO(schedule->m_em, "log: event_cb: last transfer done, kill timeout");
        }
        if (request->m_watcher) {
            net_watcher_update(request->m_watcher, 0, 0);
        }
    }
}

static void net_log_request_manage_check_multi_info(net_log_schedule_t schedule, net_log_request_manage_t mgr) {
    CURLMsg *msg;
    int msgs_left;
    CURL * handler;
    CURLcode res;
    net_log_request_t request;

    while ((msg = curl_multi_info_read(mgr->m_multi_handle, &msgs_left))) {
        handler = msg->easy_handle;
        res = msg->data.result;
        
        request = NULL;
        curl_easy_getinfo(handler, CURLINFO_PRIVATE, &request);
        assert(request);
        if (request == NULL) continue;

        switch(msg->msg) {
        case CURLMSG_DONE: {
            switch(res) {
            case CURLE_OK:
                net_log_request_complete(request, net_log_request_complete_done);
                break;
            case CURLE_OPERATION_TIMEDOUT:
                net_log_request_complete(request, net_log_request_complete_timeout);
                break;
            default:
                CPE_ERROR(
                    schedule->m_em, "log: category [%d]%s: check_multi_info done: res=%d!",
                    request->m_category->m_id, request->m_category->m_name, res);
                net_log_request_complete(request, net_log_request_complete_cancel);
                break;
            }
            break;
        }
        default:
            CPE_INFO(
                schedule->m_em, "log: category [%d]%s: check_multi_info recv msg %d!",
                request->m_category->m_id, request->m_category->m_name, msg->msg);
            break;
        }
    }
}

void net_log_request_manage_do_timeout(net_timer_t timer, void * ctx) {
    net_log_request_manage_t mgr = ctx;
    net_log_schedule_t schedule = mgr->m_schedule;
    CURLMcode rc;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "trans: net: curl_multi_socket_action(timeout)");
    }

    rc = curl_multi_socket_action(mgr->m_multi_handle, CURL_SOCKET_TIMEOUT, 0, &mgr->m_still_running);
    if (rc != CURLM_OK) {
        CPE_ERROR(schedule->m_em, "log: net: do_timer: curl_multi_socket_action return error %d!", rc);
    }

    net_log_request_manage_check_multi_info(schedule, mgr);
}

int net_log_request_manage_timer_cb(CURLM *multi, long timeout_ms, net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    if (schedule->m_debug >= 2) {
        CPE_INFO(schedule->m_em, "log: net: timer_cb: timeout_ms=%d", (int)timeout_ms);
    }

    net_timer_active(mgr->m_timer_event, (int32_t)timeout_ms);
    return 0;
}

