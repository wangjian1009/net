#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/md5.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils_sock/sock_utils.h"
#include "cpe/utils_sock/getdnssvraddrs.h"
#include "net_timer.h"
#include "net_trans_task.h"
#include "net_log_request.h"
#include "net_log_category_i.h"
#include "net_log_util.h"
#include "net_log_builder.h"
#include "lz4.h"

#define MAX_NETWORK_ERROR_SLEEP_MS 3600000
#define BASE_NETWORK_ERROR_SLEEP_MS 1000

#define MAX_QUOTA_ERROR_SLEEP_MS 60000
#define BASE_QUOTA_ERROR_SLEEP_MS 3000

#define INVALID_TIME_TRY_INTERVAL 3000

#define DROP_FAIL_DATA_TIME_SECOND (3600 * 6)

static int net_log_request_send(net_log_request_t request);
static void net_log_request_delay_commit(net_timer_t timer, void * ctx);
static void net_log_request_rebuild_time(net_log_schedule_t schedule, net_log_category_t category, net_log_request_t request, uint32_t nowTime);

static net_log_request_send_result_t net_log_request_calc_result(net_log_schedule_t schedule, net_log_request_t request);
static int32_t net_log_request_check_result(
    net_log_schedule_t schedule, net_log_category_t category, net_log_request_t request, net_log_request_send_result_t send_result);
static void net_log_request_process_result(
    net_log_schedule_t schedule, net_log_category_t category, net_log_request_manage_t mgr,
    net_log_request_t request, net_log_request_send_result_t send_result);

static net_log_request_send_result_t net_log_request_calc_result(net_log_schedule_t schedule, net_log_request_t request);

net_log_request_t
net_log_request_create(net_log_request_manage_t mgr, net_log_request_param_t send_param) {
    net_log_schedule_t schedule = mgr->m_schedule;
    net_log_category_t category = send_param->category;

    assert(send_param);

    net_log_request_t request = TAILQ_FIRST(&mgr->m_free_requests);
    if (request) {
        TAILQ_REMOVE(&mgr->m_free_requests, request, m_next);
    }
    else {
        request = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request));
        if (request == NULL) {
            CPE_ERROR(schedule->m_em, "log: %s: request: alloc fail!", mgr->m_name);
            return NULL;
        }
    }

    request->m_mgr = mgr;
    request->m_task = NULL;
    request->m_category = category;
    request->m_state = net_log_request_state_waiting;
    request->m_id = ++mgr->m_request_max_id;
    request->m_send_param = send_param;
    request->m_response_have_request_id = 0;
    request->m_last_send_error = net_log_request_send_ok;
    request->m_last_sleep_ms = 0;
    request->m_first_error_time = 0;
    request->m_delay_process = NULL;

    mgr->m_request_count++;
    mgr->m_request_buf_size += send_param->log_buf->length;
    TAILQ_INSERT_TAIL(&mgr->m_waiting_requests, request, m_next);

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: %s: category [%d]%s: request %d: created (total-active=%d, total-waiting=%d)",
            mgr->m_name, category->m_id, category->m_name, request->m_id,
            mgr->m_active_request_count, mgr->m_request_count - mgr->m_active_request_count);
    }

    return request;
}

void net_log_request_free(net_log_request_t request) {
    net_log_request_manage_t mgr = request->m_mgr;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = mgr->m_schedule;

    assert(mgr->m_request_buf_size >= request->m_send_param->log_buf->length);
    mgr->m_request_buf_size -= request->m_send_param->log_buf->length;
    assert(mgr->m_request_count > 0);
    mgr->m_request_count--;

    switch(request->m_state) {
    case net_log_request_state_waiting:
        TAILQ_REMOVE(&mgr->m_waiting_requests, request, m_next);
        break;
    case net_log_request_state_active:
        assert(mgr->m_active_request_count > 0);
        mgr->m_active_request_count--;
        TAILQ_REMOVE(&mgr->m_active_requests, request, m_next);
        break;
    }

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: %s: category [%d]%s: request %d: free  (total-active=%d, total-waiting=%d)",
            mgr->m_name, category->m_id, category->m_name, request->m_id,
            mgr->m_active_request_count, mgr->m_request_count - mgr->m_active_request_count);
    }

    if (request->m_delay_process) {
        net_timer_free(request->m_delay_process);
        request->m_delay_process = NULL;
    }

    if (request->m_task) {
        net_trans_task_free(request->m_task);
        request->m_task = NULL;
    }

    if (request->m_send_param) {
        net_log_request_param_free(request->m_send_param);
        request->m_send_param = NULL;
    }

    TAILQ_INSERT_TAIL(&mgr->m_free_requests, request, m_next);
}

void net_log_request_real_free(net_log_request_t request) {
    net_log_request_manage_t mgr = request->m_mgr;
    
    TAILQ_REMOVE(&mgr->m_free_requests, request, m_next);
    mem_free(mgr->m_schedule->m_alloc, request);
}

void net_log_request_active(net_log_request_t request) {
    net_log_request_manage_t mgr = request->m_mgr;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = mgr->m_schedule;

    if (request->m_state != net_log_request_state_active) {
        assert(request->m_state == net_log_request_state_waiting);
        net_log_request_set_state(request, net_log_request_state_active);
    }
    
    if (net_log_request_send(request) != 0) {
        net_log_request_process_result(schedule, category, mgr, request, net_log_request_send_network_error);
    }
}

void net_log_request_set_state(net_log_request_t request, net_log_request_state_t state) {
    net_log_request_manage_t mgr = request->m_mgr;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = mgr->m_schedule;

    if (request->m_state == state) return;

    if (request->m_state == net_log_request_state_active) {
        assert(mgr->m_active_request_count > 0);
        mgr->m_active_request_count--;
        TAILQ_REMOVE(&mgr->m_active_requests, request, m_next);
    }
    else {
        TAILQ_REMOVE(&mgr->m_waiting_requests, request, m_next);
    }

    net_log_request_state_t old_state = request->m_state;
    request->m_state = state;

    if (request->m_state == net_log_request_state_active) {
        mgr->m_active_request_count++;
        TAILQ_INSERT_TAIL(&mgr->m_active_requests, request, m_next);
    }
    else {
        TAILQ_INSERT_HEAD(&mgr->m_waiting_requests, request, m_next);
    }

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: %s: category [%d]%s: request %d: state %s ==> %s",
            mgr->m_name, category->m_id, category->m_name, request->m_id,
            net_log_request_state_str(old_state),
            net_log_request_state_str(request->m_state));
    }
}

static void net_log_request_commit(net_trans_task_t task, void * ctx, void * data, size_t data_size) {
    net_log_request_t request = ctx;
    net_log_category_t category = request->m_category;
    net_log_request_manage_t mgr = request->m_mgr;
    net_log_schedule_t schedule = mgr->m_schedule;
    assert(category);

    assert(request->m_task == task);
    request->m_task = NULL;

    net_log_request_send_result_t send_result = net_log_request_calc_result(schedule, request);
    net_log_request_process_result(schedule, category, mgr, request, send_result);
}

static void net_log_request_on_header(net_trans_task_t task, void * ctx, const char * name, const char * value) {
    net_log_request_t request = ctx;
    net_log_request_manage_t mgr = request->m_mgr;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = mgr->m_schedule;

    if (strcmp(name, "x-log-requestid:") == 0) {
        request->m_response_have_request_id = 1;
        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: %s: category [%d]%s: request %d: response have request-id",
                mgr->m_name, category->m_id, category->m_name, request->m_id);
        }
    }
}

static int net_log_request_send(net_log_request_t request) {
    net_log_request_manage_t mgr = request->m_mgr;
    net_log_request_param_t send_param = request->m_send_param;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = category->m_schedule;

    time_t nowTime = (uint32_t)time(NULL);
    if (nowTime - send_param->builder_time > 600
        || send_param->builder_time > (uint32_t)nowTime
        || request->m_last_send_error == net_log_request_send_time_error)
    {
        net_log_request_rebuild_time(schedule, category, request, (uint32_t)nowTime);
    }

    net_log_lz4_buf_t buffer = send_param->log_buf;
    assert(buffer);
    
    if (request->m_task) {
        net_trans_task_free(request->m_task);
        request->m_task = NULL;
    }

    request->m_response_have_request_id = 0;

    char buf[512];
    size_t sz;
    
    // url
    snprintf(
        buf, sizeof(buf), "%s://%s.%s/logstores/%s/shards/lb", 
        schedule->m_cfg_using_https ? "https" : "http",
        schedule->m_cfg_project,
        schedule->m_cfg_ep,
        category->m_name);
    
    request->m_task = net_trans_task_create(mgr->m_trans_mgr, net_trans_method_post, buf);
    if (request->m_task == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: create trans task fail",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        return -1;
    }
    
    /* struct curl_slist *connect_to = NULL; */
    /* if (schedule->m_cfg_remote_address != NULL) { */
    /*     // example.com::192.168.1.5: */
    /*     snprintf(buf, sizeof(buf), "%s.%s::%s:", schedule->m_cfg_project, schedule->m_cfg_ep, schedule->m_cfg_remote_address); */
    /*     connect_to = curl_slist_append(connect_to, buf); */
    /*     curl_easy_setopt(request->m_handler, CURLOPT_CONNECT_TO, connect_to); */
    /* } */

    /**/
    char nowTimeStr[64];
    struct tm * timeinfo = gmtime(&nowTime);
    strftime(nowTimeStr, sizeof(nowTimeStr), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);

    char md5Buf[33];
    md5Buf[32] = '\0';
    md5_to_string((const char *)buffer->data, (int)buffer->length, md5Buf);
    
    int lz4Flag = schedule->m_cfg_compress == net_log_compress_lz4;
    
    net_trans_task_append_header_line(request->m_task, "Content-Type:application/x-protobuf");
    net_trans_task_append_header_line(request->m_task, "x-log-apiversion:0.6.0");
    if (lz4Flag) {
        net_trans_task_append_header_line(request->m_task, "x-log-compresstype:lz4");
    }

    net_trans_task_append_header_line(request->m_task, "x-log-signaturemethod:hmac-sha1");

    /**/
    net_trans_task_append_header(request->m_task, "Date", nowTimeStr);

    /**/
    net_trans_task_append_header(request->m_task, "Content-MD5", md5Buf);

    /**/
    snprintf(buf, sizeof(buf), "Content-Length:%d", (int)buffer->length);
    net_trans_task_append_header_line(request->m_task, buf);

    /**/
    snprintf(buf, sizeof(buf), "x-log-bodyrawsize:%d", (int)buffer->raw_length);
    net_trans_task_append_header_line(request->m_task, buf);

    /**/
    snprintf(buf, sizeof(buf), "Host:%s.%s", schedule->m_cfg_project, schedule->m_cfg_ep);
    net_trans_task_append_header_line(request->m_task, buf);

    /*签名 */
    if (lz4Flag) {
        sz = snprintf(
            buf, sizeof(buf),
            "POST\n"
            "%s\n"
            "application/x-protobuf\n"
            "%s\n"
            "x-log-apiversion:0.6.0\n"
            "x-log-bodyrawsize:%d\n"
            "x-log-compresstype:lz4\n"
            "x-log-signaturemethod:hmac-sha1\n"
            "/logstores/%s/shards/lb",
            md5Buf, nowTimeStr, (int)buffer->raw_length, category->m_name);
    }
    else {
        sz = snprintf(
            buf, sizeof(buf),
            "POST\n"
            "%s\n"
            "application/x-protobuf\n"
            "%s\n"
            "x-log-apiversion:0.6.0\n"
            "x-log-bodyrawsize:%d\n"
            "x-log-signaturemethod:hmac-sha1\n"
            "/logstores/%s/shards/lb",
            md5Buf, nowTimeStr, (int)buffer->raw_length, category->m_name);
    }

    char sha1Buf[65];
    int sha1Len = signature_to_base64(buf, (int)sz, schedule->m_cfg_access_key, (int)strlen(schedule->m_cfg_access_key), sha1Buf);
    sha1Buf[sha1Len] = 0;

    snprintf(buf, sizeof(buf),  "Authorization:LOG %s:%s", schedule->m_cfg_access_id, sha1Buf);
    net_trans_task_append_header_line(request->m_task, buf);

    net_trans_task_set_body(request->m_task, (void *)buffer->data, (uint32_t)buffer->length);
    
    //curl_easy_setopt(request->m_handler, CURLOPT_FILETIME, 1);
    net_trans_task_set_user_agent(request->m_task, "log-c-lite_0.1.0");

    if (schedule->m_cfg_net_interface != NULL) {
        net_trans_task_set_net_interface(request->m_task, schedule->m_cfg_net_interface);
    }

    net_trans_task_set_timeout_ms(request->m_task, (schedule->m_cfg_send_timeout_s > 0 ? schedule->m_cfg_send_timeout_s : 15) * 1000);
    
    if (schedule->m_cfg_connect_timeout_s > 0) {
        net_trans_task_set_connection_timeout_ms(request->m_task, schedule->m_cfg_connect_timeout_s * 1000);
    }

    net_trans_task_set_callback(
        request->m_task,
        net_log_request_commit,
        NULL,
        NULL,
        net_log_request_on_header,
        request,
        NULL);

    /*打印调试信息 */
    if (schedule->m_debug >= 2) {
        net_trans_task_set_debug(request->m_task, schedule->m_debug - 1);
    }

    if (net_trans_task_start(request->m_task) != 0) {
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: start fail",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        net_trans_task_free(request->m_task);
        request->m_task = NULL;
        return -1;
    }

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: %s: category [%d]%s: request %d: start success",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
    }
    
    return 0;
}

static void net_log_request_process_result(
    net_log_schedule_t schedule, net_log_category_t category, net_log_request_manage_t mgr,
    net_log_request_t request, net_log_request_send_result_t send_result)
{
    int32_t sleepMs = net_log_request_check_result(schedule, category, request, send_result);
    if (sleepMs <= 0) { /*done or discard*/
        if (send_result != net_log_request_send_ok) {
            net_log_category_add_fail_statistics(category, request->m_send_param->log_count);
        }
        
        net_log_request_free(request);
        net_log_request_mgr_check_active_requests(mgr);
    }
    else if (mgr->m_state == net_log_request_manage_state_pause) {
        net_log_request_set_state(request, net_log_request_state_waiting);

        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: %s: category [%d]%s: request %d: complete with result %s, pause",
                mgr->m_name, category->m_id, category->m_name, request->m_id,
                net_log_request_send_result_str(send_result));
        }
    }
    else { /*delay process*/
        if (request->m_delay_process == NULL) {
            request->m_delay_process = net_timer_create(mgr->m_net_driver, net_log_request_delay_commit, request);
            if (request->m_delay_process == NULL) {
                CPE_ERROR(
                    schedule->m_em, "log: %s: category [%d]%s: request %d: complete with result %s, create delay process timer fail",
                    mgr->m_name, category->m_id, category->m_name, request->m_id,
                    net_log_request_send_result_str(send_result));

                net_log_category_add_fail_statistics(category, request->m_send_param->log_count);
                net_log_request_free(request);
                net_log_request_mgr_check_active_requests(mgr);
                return;
            }
        }

        net_timer_active(request->m_delay_process, sleepMs);

        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: %s: category [%d]%s: request %d: complete with result %s, delay %.2fs",
                mgr->m_name, category->m_id, category->m_name, request->m_id,
                net_log_request_send_result_str(send_result),
                ((float)sleepMs) / 1000.0f);
        }
    }
}

const char * net_log_request_send_result_str(net_log_request_send_result_t result) {
    switch(result) {
    case net_log_request_send_ok:
        return "ok";
    case net_log_request_send_network_error:
        return "network-error";
    case net_log_request_send_quota_exceed:
        return "quota-exceed";
    case net_log_request_send_unauthorized:
        return "unauthorized";
    case net_log_request_send_server_error:
        return "server-error";
    case net_log_request_send_discard_error:
        return "discard-error";
    case net_log_request_send_time_error:
        return "time-error";
    }
}

const char * net_log_request_state_str(net_log_request_state_t state) {
    switch(state) {
    case net_log_request_state_waiting:
        return "waiting";
    case net_log_request_state_active:
        return "active";
    }
}

net_log_request_param_t
net_log_request_param_create(
    net_log_category_t category, net_log_lz4_buf_t log_buf, uint32_t log_count, uint32_t builder_time)
{
    net_log_request_param_t param = (net_log_request_param_t)malloc(sizeof(struct net_log_request_param));
    assert(log_buf);
    param->category = category;
    param->log_buf = log_buf;
    param->log_count = log_count;
    param->magic_num = LOG_PRODUCER_SEND_MAGIC_NUM;
    param->builder_time = builder_time;
    return param;
}

void net_log_request_param_free(net_log_request_param_t send_param) {
    lz4_log_buf_free(send_param->log_buf);
    free(send_param);
}

static net_log_request_send_result_t
net_log_request_calc_result(net_log_schedule_t schedule, net_log_request_t request) {
    net_log_category_t category = request->m_category;
    net_log_request_manage_t mgr = request->m_mgr;
    assert(request->m_task);

    switch(net_trans_task_state(request->m_task)) {
    case net_trans_task_init:
    case net_trans_task_working:
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: transfer not complete",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        return net_log_request_send_network_error;
    case net_trans_task_done:
        break;
    }

    switch(net_trans_task_result(request->m_task)) {
    case net_trans_result_unknown:
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: transfer result unknown!",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        return net_log_request_send_network_error;
    case net_trans_result_complete:
        break;
    case net_trans_result_error:
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: transfer error, %s",
            mgr->m_name, category->m_id, category->m_name, request->m_id,
            net_trans_task_error_str(net_trans_task_error(request->m_task)));
        return net_log_request_send_network_error;
    case net_trans_result_cancel:
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: transfer canceled",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        return net_log_request_send_network_error;
    }

    int16_t http_code = net_trans_task_res_code(request->m_task);
    if (http_code == 0) {
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: transfer get server response fail",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        return net_log_request_send_network_error;
    }

    if (http_code / 100 == 2) {
        return net_log_request_send_ok;
    }
    else if (http_code >= 500 || !request->m_response_have_request_id) {
        return net_log_request_send_server_error;
    }
    else if (http_code == 403) {
        return net_log_request_send_quota_exceed;
    }
    else if (http_code == 401 || http_code == 404) {
        return net_log_request_send_unauthorized;
    }
    /* else if (result->errorMessage != NULL && strstr(result->errorMessage, LOGE_TIME_EXPIRED) != NULL) { */
    /*     return net_log_request_send_time_error; */
    /* } */
    else {
        return net_log_request_send_discard_error;
    }
}

static int32_t net_log_request_check_result(
    net_log_schedule_t schedule, net_log_category_t category, net_log_request_t request, net_log_request_send_result_t send_result)
{
    net_log_request_manage_t mgr = request->m_mgr;

    switch (send_result) {
    case net_log_request_send_ok:
        break;
    case net_log_request_send_time_error:
        // if no this marco, drop data
        request->m_last_send_error = net_log_request_send_time_error;
        request->m_last_sleep_ms = INVALID_TIME_TRY_INTERVAL;

        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: %s: category [%d]%s: request %d: time error",
                mgr->m_name, category->m_id, category->m_name, request->m_id);
        }
        
        return request->m_last_sleep_ms;
    case net_log_request_send_quota_exceed:
        if (request->m_last_send_error != net_log_request_send_quota_exceed) {
            request->m_last_send_error = net_log_request_send_quota_exceed;
            request->m_last_sleep_ms = BASE_QUOTA_ERROR_SLEEP_MS;
            request->m_first_error_time = (uint32_t)time(NULL);
        }
        else {
            if (request->m_last_sleep_ms < MAX_QUOTA_ERROR_SLEEP_MS) {
                request->m_last_sleep_ms *= 2;
            }

            if (time(NULL) - request->m_first_error_time > DROP_FAIL_DATA_TIME_SECOND) {
                break;
            }
        }
        
        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: %s: category [%d]%s: request %d: send quota error",
                mgr->m_name, category->m_id, category->m_name, request->m_id);
        }

        return request->m_last_sleep_ms;
    case net_log_request_send_server_error:
    case net_log_request_send_network_error:
        if (request->m_last_send_error != net_log_request_send_network_error) {
            request->m_last_send_error = net_log_request_send_network_error;
            request->m_last_sleep_ms = BASE_NETWORK_ERROR_SLEEP_MS;
            request->m_first_error_time = (uint32_t)time(NULL);
        }
        else {
            if (request->m_last_sleep_ms < MAX_NETWORK_ERROR_SLEEP_MS) {
                request->m_last_sleep_ms *= 2;
            }
            // only drop data when SEND_TIME_INVALID_FIX not defined
            if (time(NULL) - request->m_first_error_time > DROP_FAIL_DATA_TIME_SECOND) {
                break;
            }
        }

        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: %s: category [%d]%s: request %d: send network error",
                mgr->m_name, category->m_id, category->m_name, request->m_id);
        }

        return request->m_last_sleep_ms;
    default:
        // discard data
        break;
    }

    if (schedule->m_debug) {
        if (send_result == net_log_request_send_ok) {
            CPE_INFO(
                schedule->m_em, "log: %s: category [%d]%s: request %d: send success, buffer-len=%d, raw-len=%d",
                mgr->m_name, category->m_id, category->m_name, request->m_id,
                (int)request->m_send_param->log_buf->length,
                (int)request->m_send_param->log_buf->raw_length);
        }
        else {
            CPE_INFO(
                schedule->m_em, "log: %s: category [%d]%s: request %d: send fail, discard data, buffer-len=%d, raw=len=%d",
                mgr->m_name, category->m_id, category->m_name, request->m_id,
                (int)request->m_send_param->log_buf->length,
                (int)request->m_send_param->log_buf->raw_length);
        }
    }

    return 0;
}

static const char * net_log_request_dump(
    net_log_schedule_t schedule, net_log_request_manage_t mgr, char *i_ptr, size_t size, char nohex)
{
    unsigned char * ptr = (unsigned char *)i_ptr;
    size_t i;
    size_t c;

    mem_buffer_t buffer = mgr->m_tmp_buffer;
    mem_buffer_clear_data(buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    
    unsigned int width = 0x10;
 
    if(nohex) {
        /* without the hex output, we can fit more on screen */ 
        width = 0x40;
    }
    
    for(i = 0; i < size; i += width) {
        if (i != 0) stream_printf((write_stream_t)&ws, "\n");

        stream_printf((write_stream_t)&ws, "%4.4lx: ", (unsigned long)i);
 
        if(!nohex) {
            /* hex not disabled, show it */ 
            for(c = 0; c < width; c++) {
                if(i + c < size) {
                    stream_printf((write_stream_t)&ws, "%02x ", ptr[i + c]);
                }
                else {
                    stream_printf((write_stream_t)&ws, "   ");
                }
            }
        }
 
        for (c = 0; (c < width) && (i + c < size); c++) {
            /* check for 0D0A; if found, skip past and start a new line of output */ 
            if (nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D && ptr[i + c + 1] == 0x0A) {
                i += (c + 2 - width);
                break;
            }
            stream_printf((write_stream_t)&ws, "%c", (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
            /* check again for 0D0A, to avoid an extra \n if it's at width */ 
            if (nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D && ptr[i + c + 2] == 0x0A) {
                i += (c + 3 - width);
                break;
            }
        }
    }

    stream_putc((write_stream_t)&ws, 0);

    return (const char *)mem_buffer_make_continuous(buffer, 0);
}

static void net_log_request_delay_commit(net_timer_t timer, void * ctx) {
    net_log_request_t request = ctx;
    net_log_request_manage_t mgr = request->m_mgr;
    net_log_schedule_t schedule = mgr->m_schedule;
    net_log_category_t category = request->m_category;

    if (net_log_request_send(request) != 0) {
        net_log_request_process_result(schedule, category, mgr, request, net_log_request_send_network_error);
    }
}

static void net_log_request_rebuild_time(
    net_log_schedule_t schedule, net_log_category_t category, net_log_request_t request, uint32_t nowTime)
{
    net_log_request_manage_t mgr = request->m_mgr;

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: %s: category [%d]%s: request %d: rebuild time",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
    }

    net_log_lz4_buf_t lz4_buf = request->m_send_param->log_buf;
    
    char * buf = (char *)malloc(lz4_buf->raw_length);
    if (buf == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: alloc buf fail",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        return;
    }

    if (LZ4_decompress_safe((const char* )lz4_buf->data, buf, lz4_buf->length, lz4_buf->raw_length) <= 0) {
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: LZ4_decompress_safe error",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        free(buf);
        return;
    }

    fix_log_group_time(buf, lz4_buf->raw_length, nowTime);

    int compress_bound = LZ4_compressBound(lz4_buf->raw_length);
    char *compress_data = (char *)malloc(compress_bound);
    int compressed_size = LZ4_compress_default((char *)buf, compress_data, lz4_buf->raw_length, compress_bound);
    if(compressed_size <= 0) {
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: LZ4_compress_default error",
            mgr->m_name, category->m_id, category->m_name, request->m_id);
        free(buf);
        free(compress_data);
        return;
    }
    free(buf); buf = NULL;

    net_log_lz4_buf_t new_lz4_buf = lz4_log_buf_create(schedule, compress_data, compressed_size, lz4_buf->raw_length);
    free(compress_data); compress_data = NULL;
    if (new_lz4_buf == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: %s: category [%d]%s: request %d: alloc new buf fail, sz=%d",
            mgr->m_name, category->m_id, category->m_name, request->m_id,
            (int)(sizeof(struct net_log_lz4_buf) + compressed_size));
        return;
    }

    lz4_log_buf_free(request->m_send_param->log_buf);
    request->m_send_param->log_buf = new_lz4_buf;
    request->m_send_param->builder_time = nowTime;
}
