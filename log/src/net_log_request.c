#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/md5.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils_sock/sock_utils.h"
#include "cpe/utils_sock/getdnssvraddrs.h"
#include "net_trans_task.h"
#include "net_log_request.h"
#include "net_log_env_i.h"
#include "net_log_category_i.h"
#include "net_log_util.h"
#include "net_log_builder.h"
#include "net_log_thread_cmd.h"
#include "lz4.h"

static int net_log_request_send(net_log_request_t request);
static void net_log_request_rebuild_time(net_log_schedule_t schedule, net_log_category_t category, net_log_request_t request, uint32_t nowTime);
static void net_log_request_statistic_trans_error(
    net_log_schedule_t schedule, net_log_category_t category, net_log_thread_t log_thread,
    net_trans_task_error_t trans_error);
static void net_log_request_statistic_http_error(
    net_log_schedule_t schedule, net_log_category_t category, net_log_thread_t log_thread,
    int16_t http_code, const char * http_msg);

net_log_request_t
net_log_request_create(net_log_thread_t log_thread, net_log_request_param_t send_param) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    net_log_category_t category = send_param->category;

    assert(send_param);

    net_log_request_t request = TAILQ_FIRST(&log_thread->m_free_requests);
    if (request) {
        TAILQ_REMOVE(&log_thread->m_free_requests, request, m_next);
    }
    else {
        request = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request));
        if (request == NULL) {
            CPE_ERROR(schedule->m_em, "log: thread %s: request: alloc fail!", log_thread->m_name);
            return NULL;
        }
    }

    request->m_thread = log_thread;
    request->m_task = NULL;
    request->m_category = category;
    request->m_state = net_log_request_state_waiting;
    request->m_id = ++log_thread->m_request_max_id;
    request->m_send_param = send_param;
    request->m_response_have_request_id = 0;
    request->m_need_rebuild_time = 0;

    log_thread->m_request_count++;
    log_thread->m_request_buf_size += send_param->log_buf->length;
    TAILQ_INSERT_TAIL(&log_thread->m_waiting_requests, request, m_next);

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: created (total-active=%d, total-waiting=%d)",
            log_thread->m_name, category->m_id, category->m_name, request->m_id,
            log_thread->m_active_request_count, log_thread->m_request_count - log_thread->m_active_request_count);
    }

    return request;
}

void net_log_request_free(net_log_request_t request) {
    net_log_thread_t log_thread = request->m_thread;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = log_thread->m_schedule;

    ASSERT_ON_THREAD(log_thread);
    
    assert(log_thread->m_request_buf_size >= request->m_send_param->log_buf->length);
    log_thread->m_request_buf_size -= request->m_send_param->log_buf->length;
    assert(log_thread->m_request_count > 0);
    log_thread->m_request_count--;

    switch(request->m_state) {
    case net_log_request_state_waiting:
        TAILQ_REMOVE(&log_thread->m_waiting_requests, request, m_next);
        break;
    case net_log_request_state_active:
        assert(log_thread->m_active_request_count > 0);
        log_thread->m_active_request_count--;
        TAILQ_REMOVE(&log_thread->m_active_requests, request, m_next);
        break;
    }

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: free  (total-active=%d, total-waiting=%d)",
            log_thread->m_name, category->m_id, category->m_name, request->m_id,
            log_thread->m_active_request_count, log_thread->m_request_count - log_thread->m_active_request_count);
    }

    if (request->m_task) {
        net_trans_task_clear_callback(request->m_task);
        net_trans_task_free(request->m_task);
        request->m_task = NULL;
    }

    if (request->m_send_param) {
        net_log_request_param_free(request->m_send_param);
        request->m_send_param = NULL;
    }

    TAILQ_INSERT_TAIL(&log_thread->m_free_requests, request, m_next);
}

void net_log_request_real_free(net_log_request_t request) {
    net_log_thread_t log_thread = request->m_thread;
    ASSERT_ON_THREAD(log_thread);
    
    TAILQ_REMOVE(&log_thread->m_free_requests, request, m_next);
    mem_free(log_thread->m_schedule->m_alloc, request);
}

void net_log_request_active(net_log_request_t request) {
    net_log_thread_t log_thread = request->m_thread;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    if (request->m_state != net_log_request_state_active) {
        assert(request->m_state == net_log_request_state_waiting);
        net_log_request_set_state(request, net_log_request_state_active);
    }
    
    if (net_log_request_send(request) != 0) {
        net_log_request_set_state(request, net_log_request_state_waiting);
        net_log_thread_commit_schedule_delay(log_thread, net_log_thread_commit_error_network);
    }
}

void net_log_request_cancel(net_log_request_t request) {
    net_log_thread_t log_thread = request->m_thread;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    if (request->m_task) {
        net_trans_task_clear_callback(request->m_task);
        net_trans_task_free(request->m_task);
        request->m_task = NULL;
    }
    
    net_log_request_set_state(request, net_log_request_state_waiting);
}

void net_log_request_set_state(net_log_request_t request, net_log_request_state_t state) {
    net_log_thread_t log_thread = request->m_thread;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    if (request->m_state == state) return;

    if (request->m_state == net_log_request_state_active) {
        assert(log_thread->m_active_request_count > 0);
        log_thread->m_active_request_count--;
        TAILQ_REMOVE(&log_thread->m_active_requests, request, m_next);
    }
    else {
        TAILQ_REMOVE(&log_thread->m_waiting_requests, request, m_next);
    }

    net_log_request_state_t old_state = request->m_state;
    request->m_state = state;

    if (request->m_state == net_log_request_state_active) {
        log_thread->m_active_request_count++;
        TAILQ_INSERT_TAIL(&log_thread->m_active_requests, request, m_next);
    }
    else {
        TAILQ_INSERT_HEAD(&log_thread->m_waiting_requests, request, m_next);
    }

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: state %s ==> %s",
            log_thread->m_name, category->m_id, category->m_name, request->m_id,
            net_log_request_state_str(old_state),
            net_log_request_state_str(request->m_state));
    }
}

static void net_log_request_commit_do_error(
    net_log_schedule_t schedule, net_log_category_t category, net_log_thread_t log_thread, 
    net_log_request_t request, net_log_thread_commit_error_t commit_error)
{
    net_log_request_set_state(request, net_log_request_state_waiting);
    
    if (commit_error != net_log_thread_commit_error_none) {
        net_log_thread_commit_schedule_delay(log_thread, commit_error);
    }
}

static void net_log_request_commit_do_success(
    net_log_schedule_t schedule, net_log_category_t category, net_log_thread_t log_thread, net_log_request_t request)
{
    net_log_category_statistic_success(category, log_thread);
    net_log_request_free(request);
}

static void net_log_request_commit(net_trans_task_t task, void * ctx, void * data, size_t data_size) {
    net_log_request_t request = ctx;
    net_log_category_t category = request->m_category;
    net_log_thread_t log_thread = request->m_thread;
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    assert(category);

    assert(request->m_task == task);

    switch(net_trans_task_state(task)) {
    case net_trans_task_init:
    case net_trans_task_working:
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: transfer not complete",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
        net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_none);
        goto COMMIT_COMPLETE;
    case net_trans_task_done:
        break;
    }

    switch(net_trans_task_result(task)) {
    case net_trans_result_unknown:
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: transfer result unknown!",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
        net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_network);
        goto COMMIT_COMPLETE;
    case net_trans_result_complete:
        break;
    case net_trans_result_error: {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: transfer error, %s",
            log_thread->m_name, category->m_id, category->m_name, request->m_id,
            net_trans_task_error_str(net_trans_task_error(task)));
        net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_network);
        net_log_request_statistic_trans_error(schedule, category, log_thread, net_trans_task_error(task));
        goto COMMIT_COMPLETE;
    }
    case net_trans_result_cancel:
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: transfer canceled",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
        net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_none);
        goto COMMIT_COMPLETE;
    }

    int16_t http_code = net_trans_task_res_code(task);
    if (http_code == 0) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: transfer get server response fail",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
        net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_network);
        goto COMMIT_COMPLETE;
    }

    if (http_code / 100 == 2) {
        net_log_request_commit_do_success(schedule, category, log_thread, request);
    }
    else {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: server http error code %d",
            log_thread->m_name, category->m_id, category->m_name, request->m_id, http_code);

        if (http_code >= 500 || !request->m_response_have_request_id) {
            net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_network);
        } else if (http_code == 403) {
            net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_quota_exceed);
        } else if (http_code == 401 || http_code == 404) {
            net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_network);
        }
        /* else if (result->errorMessage != NULL && strstr(result->errorMessage, LOGE_TIME_EXPIRED) != NULL) { */
        /*     return net_log_request_send_time_error; */
        /* } */
        else {
            net_log_request_commit_do_error(schedule, category, log_thread, request, net_log_thread_commit_error_network);
        }

        net_log_request_statistic_http_error(schedule, category, log_thread, http_code, net_trans_task_error_addition(task));
    }
    
COMMIT_COMPLETE:    
    net_trans_task_free(task);
    request->m_task = NULL;

    net_log_thread_check_active_requests(log_thread);
}

static void net_log_request_on_header(net_trans_task_t task, void * ctx, const char * name, const char * value) {
    net_log_request_t request = ctx;
    net_log_thread_t log_thread = request->m_thread;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    if (strcmp(name, "x-log-requestid:") == 0) {
        request->m_response_have_request_id = 1;
        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: thread %s: category [%d]%s: request %d: response have request-id",
                log_thread->m_name, category->m_id, category->m_name, request->m_id);
        }
    }
}

static int net_log_request_send(net_log_request_t request) {
    net_log_thread_t log_thread = request->m_thread;
    net_log_request_param_t send_param = request->m_send_param;
    net_log_category_t category = request->m_category;
    net_log_schedule_t schedule = category->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    assert(!net_log_thread_is_suspend(log_thread));
    
    time_t nowTime = (uint32_t)time(NULL);
    if (nowTime - send_param->builder_time > 600
        || send_param->builder_time > (uint32_t)nowTime
        || request->m_need_rebuild_time)
    {
        net_log_request_rebuild_time(schedule, category, request, (uint32_t)nowTime);
        request->m_need_rebuild_time = 0;
    }

    net_log_lz4_buf_t buffer = send_param->log_buf;
    assert(buffer);

    if (request->m_task) {
        net_trans_task_clear_callback(request->m_task);
        net_trans_task_free(request->m_task);
        request->m_task = NULL;
    }

    request->m_response_have_request_id = 0;

    char buf[512];
    size_t sz;
    
    // url
    snprintf(
        buf, sizeof(buf), "%s/logstores/%s/shards/lb", 
        log_thread->m_env_active->m_url,
        category->m_name);
    
    request->m_task = net_trans_task_create(log_thread->m_trans_mgr, net_trans_method_post, buf, schedule->m_debug);
    if (request->m_task == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: create trans task fail",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
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
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: start fail",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
        net_trans_task_free(request->m_task);
        request->m_task = NULL;
        return -1;
    }

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: start success",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
    }
    
    return 0;
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

static void net_log_request_rebuild_time(
    net_log_schedule_t schedule, net_log_category_t category, net_log_request_t request, uint32_t nowTime)
{
    net_log_thread_t log_thread = request->m_thread;
    ASSERT_ON_THREAD(log_thread);
    
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: rebuild time",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
    }

    net_log_lz4_buf_t lz4_buf = request->m_send_param->log_buf;
    
    char * buf = (char *)malloc(lz4_buf->raw_length);
    if (buf == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: alloc buf fail",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
        return;
    }

    if (LZ4_decompress_safe((const char* )lz4_buf->data, buf, lz4_buf->length, lz4_buf->raw_length) <= 0) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: LZ4_decompress_safe error",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
        free(buf);
        return;
    }

    fix_log_group_time(buf, lz4_buf->raw_length, nowTime);

    int compress_bound = LZ4_compressBound(lz4_buf->raw_length);
    char *compress_data = (char *)malloc(compress_bound);
    int compressed_size = LZ4_compress_default((char *)buf, compress_data, lz4_buf->raw_length, compress_bound);
    if(compressed_size <= 0) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: LZ4_compress_default error",
            log_thread->m_name, category->m_id, category->m_name, request->m_id);
        free(buf);
        free(compress_data);
        return;
    }
    free(buf); buf = NULL;

    net_log_lz4_buf_t new_lz4_buf = lz4_log_buf_create(schedule, compress_data, compressed_size, lz4_buf->raw_length);
    free(compress_data); compress_data = NULL;
    if (new_lz4_buf == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: request %d: alloc new buf fail, sz=%d",
            log_thread->m_name, category->m_id, category->m_name, request->m_id,
            (int)(sizeof(struct net_log_lz4_buf) + compressed_size));
        return;
    }

    assert(log_thread->m_request_buf_size >= request->m_send_param->log_buf->length);
    log_thread->m_request_buf_size -= request->m_send_param->log_buf->length;
    lz4_log_buf_free(request->m_send_param->log_buf);

    request->m_send_param->log_buf = new_lz4_buf;
    request->m_send_param->builder_time = nowTime;
    log_thread->m_request_buf_size += request->m_send_param->log_buf->length;
}

static void net_log_request_statistic_trans_error(
    net_log_schedule_t schedule, net_log_category_t category, net_log_thread_t log_thread,
    net_trans_task_error_t trans_error)
{
    struct net_log_thread_cmd_staistic_op_error op_error_cmd;
    op_error_cmd.head.m_size = sizeof(op_error_cmd) + 1;
    op_error_cmd.head.m_cmd = net_log_thread_cmd_type_staistic_op_error;
    op_error_cmd.m_env = log_thread->m_env_active;
    op_error_cmd.m_category = category;
    op_error_cmd.m_trans_error = trans_error;
    op_error_cmd.m_http_code = 0;
    op_error_cmd.m_http_msg[0] = 0;

    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&op_error_cmd, log_thread);
}

static void net_log_request_statistic_http_error(
    net_log_schedule_t schedule, net_log_category_t category, net_log_thread_t log_thread,
    int16_t http_code, const char * http_msg)
{
    char cmd_buf[sizeof(struct net_log_thread_cmd_staistic_op_error) + 128];
    
    size_t http_msg_len = http_msg ? strlen(http_msg) : 0;
    if (http_msg_len > 128) {
        http_msg_len = 128;
    }
    
    struct net_log_thread_cmd_staistic_op_error * op_error_cmd = (struct net_log_thread_cmd_staistic_op_error *)cmd_buf;
    op_error_cmd->head.m_size = sizeof(*op_error_cmd) + http_msg_len + 1;
    op_error_cmd->head.m_cmd = net_log_thread_cmd_type_staistic_op_error;
    op_error_cmd->m_env = log_thread->m_env_active;
    op_error_cmd->m_category = category;
    op_error_cmd->m_trans_error = net_trans_task_error_none;
    op_error_cmd->m_http_code = http_code;
    if (http_msg) {
        memcpy(op_error_cmd->m_http_msg, http_msg, http_msg_len);
    }
    op_error_cmd->m_http_msg[http_msg_len] = 0;

    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)op_error_cmd, log_thread);
}
