#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/time_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils_sock/sock_utils.h"
#include "cpe/utils_sock/getdnssvraddrs.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_watcher.h"
#include "net_trans_task_i.h"

static size_t net_trans_task_write_cb(char *ptr, size_t size, size_t nmemb, void * userdata);
static int net_trans_task_prog_cb(void *p, double dltotal, double dlnow, double ult, double uln);
static size_t net_trans_task_header_cb(void *ptr, size_t size, size_t nmemb, void *stream);
static int net_trans_task_sock_config_cb(void *p, curl_socket_t curlfd, curlsocktype purpose);

static int net_trans_task_setup_url_origin(net_trans_manage_t mgr, net_trans_task_t task, const char * uri);
static int net_trans_task_setup_url_ipv6(
    net_trans_manage_t mgr, net_trans_task_t task, const char * uri,
    const char * protocol, net_address_t host, const char * uri_left);
static int net_trans_task_setup_url_ipv4(
    net_trans_manage_t mgr, net_trans_task_t task, const char * uri,
    const char * protocol, net_address_t host, const char * uri_left);
static int net_trans_task_setup_dns(net_trans_manage_t mgr, net_trans_task_t task, net_local_ip_stack_t ip_stack);

net_trans_task_t
net_trans_task_create(net_trans_manage_t mgr, net_trans_method_t method, const char * uri, uint8_t is_debug) {
    net_trans_task_t task = NULL;
    net_address_t target_host = NULL;

    net_local_ip_stack_t ip_stack = net_schedule_local_ip_stack(mgr->m_schedule);
    if (ip_stack == net_local_ip_stack_none) {
        CPE_ERROR(mgr->m_em, "trans: %s-: ip stack non, can`t create task!", mgr->m_name);
        return NULL;
    }

    char protocol[32];
    const char * host_begin = strstr(uri, "://");
    if (host_begin == NULL) {
        CPE_ERROR(mgr->m_em, "trans: %s-: url '%s' format error!", mgr->m_name, uri);
        return NULL;
    }
    cpe_str_dup_range(protocol, sizeof(protocol), uri, host_begin);

    host_begin += 3;
    
    const char * host_end = host_begin;
    for(; *host_end != 0; host_end++) {
        char c = *host_end;
        if (c ==  '@') {
            host_begin = host_end + 1;
        }
        else if (c == '/' || c == ':') {
            break;
        }
    }

    char buf[128];
    cpe_str_dup_range(buf, sizeof(buf), host_begin, host_end);

    target_host = net_address_create_auto(mgr->m_schedule, buf);
    if (target_host == NULL) {
        CPE_ERROR(mgr->m_em, "trans: %s-: create host address from %s fail, uri=%s", mgr->m_name, buf, uri);
        return NULL;
    }

    task = TAILQ_FIRST(&mgr->m_free_tasks);
    if (task) {
        TAILQ_REMOVE(&mgr->m_free_tasks, task, m_next_for_mgr);
    }
    else {
        task = mem_alloc(mgr->m_alloc, sizeof(struct net_trans_task));
        if (task == NULL) {
            CPE_ERROR(mgr->m_em, "trans: %s-: alloc fail", mgr->m_name);
            goto CREATED_ERROR;
        }
    }

    task->m_mgr = mgr;
    task->m_watcher = NULL;
    task->m_id = ++mgr->m_max_task_id;
    task->m_state = net_trans_task_init;
    task->m_result = net_trans_result_unknown;
    task->m_error = net_trans_task_error_none;
    task->m_error_addition = NULL;

    task->m_header = NULL;
    task->m_connect_to = NULL;
    task->m_cfg_protect_vpn = mgr->m_cfg_protect_vpn;
    task->m_cfg_explicit_dns = 0;
#if TARGET_OS_IPHONE
    task->m_cfg_explicit_dns = 1;
#endif
    task->m_debug = 0;

    task->m_handler = curl_easy_init();
    if (task->m_handler == NULL) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: curl_easy_init fail!", mgr->m_name, task->m_id);
        goto CREATED_ERROR;
    }

    task->m_read_cb = NULL;
    task->m_read_ctx = NULL;
    task->m_read_ctx_free = NULL;

    task->m_trace_begin_time_ms = 0;
    task->m_trace_last_time_ms = 0;
    
    task->m_in_callback = 0;
    task->m_is_free = 0;
    task->m_commit_op = NULL;
    task->m_write_op = NULL;
    task->m_head_op = NULL;
    task->m_progress_op = NULL;
    task->m_ctx = NULL;
    task->m_ctx_free = NULL;

    net_trans_task_set_debug(task, is_debug);

    if (curl_easy_setopt(task->m_handler, CURLOPT_PRIVATE, task) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_NOSIGNAL, 1) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_TCP_NODELAY, 1) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_NETRC, CURL_NETRC_IGNORED) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_WRITEFUNCTION, net_trans_task_write_cb) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_WRITEDATA, task) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_NOPROGRESS, 0L) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_PROGRESSFUNCTION, net_trans_task_prog_cb) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_PROGRESSDATA, task) != CURLE_OK
        )
    {
        goto CREATED_ERROR;
    }

    switch(net_address_type(target_host)) {
    case net_address_ipv4:
        assert(ip_stack != net_local_ip_stack_none);
        if (ip_stack == net_local_ip_stack_ipv6) {
            if (net_trans_task_setup_url_ipv6(mgr, task, uri, protocol, target_host, host_end) != 0) goto CREATED_ERROR;
        }
        else {
            if (net_trans_task_setup_url_origin(mgr, task, uri) != 0) goto CREATED_ERROR;
        }
        break;
    case net_address_ipv6:
        if (ip_stack == net_local_ip_stack_ipv4) {
            if (net_trans_task_setup_url_ipv4(mgr, task, uri, protocol, target_host, host_end) != 0) goto CREATED_ERROR;
        }
        else {
            if (net_trans_task_setup_url_origin(mgr, task, uri) != 0) goto CREATED_ERROR;
        }
        break;
    case net_address_domain:
        if (net_trans_task_setup_url_origin(mgr, task, uri) != 0) goto CREATED_ERROR;

        if (ip_stack == net_local_ip_stack_dual) {
            if (curl_easy_setopt(task->m_handler, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER) != (int)CURLE_OK) goto CREATED_ERROR;
        }
        else if (ip_stack ==  net_local_ip_stack_ipv4) {
            if (curl_easy_setopt(task->m_handler, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4) != (int)CURLE_OK) goto CREATED_ERROR;
        }
        else if (ip_stack == net_local_ip_stack_ipv6) {
            if (curl_easy_setopt(task->m_handler, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6) != (int)CURLE_OK) goto CREATED_ERROR;
        }
        break;
    case net_address_local:
        break;
    }

    switch(method) {
    case net_trans_method_get:
        if (curl_easy_setopt(task->m_handler, CURLOPT_CUSTOMREQUEST, "GET") != (int)CURLE_OK) goto CREATED_ERROR;
        break;
    case net_trans_method_post:
        if (curl_easy_setopt(task->m_handler, CURLOPT_POST, 1L) != (int)CURLE_OK) goto CREATED_ERROR;
        break;
    case net_trans_method_put:
        if (curl_easy_setopt(task->m_handler, CURLOPT_PUT, 1L) != (int)CURLE_OK) goto CREATED_ERROR;
        break;
    case net_trans_method_delete:
        if (curl_easy_setopt(task->m_handler, CURLOPT_CUSTOMREQUEST, "DELETE") != (int)CURLE_OK) goto CREATED_ERROR;
        break;
    case net_trans_method_patch:
        if (curl_easy_setopt(task->m_handler, CURLOPT_CUSTOMREQUEST, "PATCH") != (int)CURLE_OK) goto CREATED_ERROR;
        break;
    }

    if (strcasecmp(protocol, "https") == 0) {
        if (curl_easy_setopt(task->m_handler, CURLOPT_SSL_VERIFYPEER, 0)
            || curl_easy_setopt(task->m_handler, CURLOPT_SSL_VERIFYHOST, 0))
        {
            goto CREATED_ERROR;
        }
    }

    cpe_hash_entry_init(&task->m_hh_for_mgr);
    if (cpe_hash_table_insert_unique(&mgr->m_tasks, task) != 0) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: id duplicate!", mgr->m_name, task->m_id);
        goto CREATED_ERROR;
    }

    mem_buffer_init(&task->m_buffer, mgr->m_alloc);
    task->m_target_address = target_host;

    return task;

CREATED_ERROR:
    if (target_host) {
        net_address_free(target_host);
    }
    
    if (task) {
        if (task->m_handler) {
            curl_easy_cleanup(task->m_handler);
            task->m_handler = NULL;
        }

        TAILQ_INSERT_TAIL(&mgr->m_free_tasks, task, m_next_for_mgr);
        task = NULL;
    }
    
    return NULL;
}

void net_trans_task_free(net_trans_task_t task) {
    net_trans_manage_t mgr = task->m_mgr;

    if (task->m_in_callback) {
        task->m_is_free = 1;
        return;
    }

    if (task->m_state == net_trans_task_working) {
        net_trans_task_set_done(task, net_trans_result_cancel, net_trans_task_error_internal, "cancel-in-working");
        return;
    }

    if (mgr->m_debug || task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: free!", mgr->m_name, task->m_id);
    }

    curl_easy_cleanup(task->m_handler);
    task->m_handler = NULL;

    if (task->m_watcher) {
        net_watcher_free(task->m_watcher);
        task->m_watcher = NULL;
    }

    if (task->m_read_ctx_free) {
        task->m_read_ctx_free(task->m_read_ctx);
        task->m_read_ctx = NULL;
        task->m_read_ctx_free = NULL;
    }

    if (task->m_ctx_free) {
        task->m_ctx_free(task->m_ctx);
        task->m_ctx = NULL;
        task->m_ctx_free = NULL;
    }

    if (task->m_target_address) {
        net_address_free(task->m_target_address);
        task->m_target_address = NULL;
    }

    mem_buffer_clear(&task->m_buffer);

    if (task->m_header) {
        curl_slist_free_all(task->m_header);
        task->m_header = NULL;
    }
    
    if (task->m_connect_to) {
        curl_slist_free_all(task->m_connect_to);
        task->m_connect_to = NULL;
    }

    if (task->m_error_addition) {
        mem_free(mgr->m_alloc,  task->m_error_addition);
        task->m_error_addition = NULL;
    }

    cpe_hash_table_remove_by_ins(&mgr->m_tasks, task);

    TAILQ_INSERT_TAIL(&mgr->m_free_tasks, task, m_next_for_mgr);
}

void net_trans_task_free_all(net_trans_manage_t mgr) {
    struct cpe_hash_it task_it;
    net_trans_task_t task;

    cpe_hash_it_init(&task_it, &mgr->m_tasks);

    task = cpe_hash_it_next(&task_it);
    while(task) {
        net_trans_task_t next = cpe_hash_it_next(&task_it);
        net_trans_task_free(task);
        task = next;
    }
}

void net_trans_task_real_free(net_trans_task_t task) {
    net_trans_manage_t mgr = task->m_mgr;

    TAILQ_REMOVE(&mgr->m_free_tasks, task, m_next_for_mgr);

    mem_free(mgr->m_alloc, task);
}

uint32_t net_trans_task_id(net_trans_task_t task) {
    return task->m_id;
}

net_trans_task_t net_trans_task_find(net_trans_manage_t mgr, uint32_t task_id) {
    struct net_trans_task key;
    key.m_id = task_id;
    return cpe_hash_table_find(&mgr->m_tasks, &key);
}

net_trans_manage_t net_trans_task_manage(net_trans_task_t task) {
    return task->m_mgr;
}

net_address_t net_trans_task_target_address(net_trans_task_t task) {
    return task->m_target_address;
}

net_trans_task_state_t net_trans_task_state(net_trans_task_t task) {
    return task->m_state;
}

net_trans_task_result_t net_trans_task_result(net_trans_task_t task) {
    return task->m_result;
}

net_trans_task_error_t net_trans_task_error(net_trans_task_t task) {
    return task->m_error;
}

const char * net_trans_task_error_addition(net_trans_task_t task) {
    return task->m_error_addition;
}

int16_t net_trans_task_res_code(net_trans_task_t task) {
    long http_code = 0;
    CURLcode rc = curl_easy_getinfo(task->m_handler, CURLINFO_RESPONSE_CODE, &http_code);
    if (rc != CURLE_OK) {
        CPE_ERROR(
            task->m_mgr->m_em, "trans: %s-%d: get res code: error, rc=%d (%s)!",
            task->m_mgr->m_name, task->m_id, rc, curl_easy_strerror(rc));
        return -1;
    }
    
    return (int16_t)http_code;
}

int net_trans_task_set_skip_data(net_trans_task_t task, ssize_t skip_length) {
    net_trans_manage_t mgr = task->m_mgr;

    if (curl_easy_setopt(task->m_handler, CURLOPT_RESUME_FROM_LARGE, skip_length > 0 ? (curl_off_t)skip_length : (curl_off_t)0) != CURLE_OK) {
        CPE_ERROR(
            mgr->m_em, "trans: %s-%d: set skip data %d error!",
            mgr->m_name, task->m_id, (int)skip_length);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: set skip data %d!", mgr->m_name, task->m_id, (int)skip_length);
    }

    return 0;
}

int net_trans_task_set_timeout_ms(net_trans_task_t task, uint64_t timeout_ms) {
    net_trans_manage_t mgr = task->m_mgr;

	if (curl_easy_setopt(task->m_handler, CURLOPT_TIMEOUT_MS, timeout_ms) != CURLE_OK) {
        CPE_ERROR(
            mgr->m_em, "trans: %s-%d: set timeout " FMT_UINT64_T "ms error!",
            mgr->m_name, task->m_id, timeout_ms);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(
            mgr->m_em, "trans: %s-%d: set timeout " FMT_UINT64_T "ms!",
            mgr->m_name, task->m_id, timeout_ms);
    }

    return 0;
}

int net_trans_task_set_connection_timeout_ms(net_trans_task_t task, uint64_t timeout_ms) {
    net_trans_manage_t mgr = task->m_mgr;

	if (curl_easy_setopt(task->m_handler, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms) != CURLE_OK) {
        CPE_ERROR(
            mgr->m_em, "trans: %s-%d: set connection-timeout " FMT_UINT64_T "ms error!",
            mgr->m_name, task->m_id, timeout_ms);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(
            mgr->m_em, "trans: %s-%d: set connection-timeout " FMT_UINT64_T "ms!",
            mgr->m_name, task->m_id, timeout_ms);
    }

    return 0;
}

int net_trans_task_set_useragent(net_trans_task_t task, const char * agent) {
    net_trans_manage_t mgr = task->m_mgr;

	if (curl_easy_setopt(task->m_handler, CURLOPT_USERAGENT, agent) != CURLE_OK) {
        CPE_ERROR(
            mgr->m_em, "trans: %s-%d: set useragent %s error!",
            mgr->m_name, task->m_id, agent);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: set useragent %s!", mgr->m_name, task->m_id, agent);
    }

    return 0;
}

int net_trans_task_append_header(net_trans_task_t task, const char * name, const char * value) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s:%s", name, value);
    return net_trans_task_append_header_line(task, buf);
}

int net_trans_task_append_header_line(net_trans_task_t task, const char * header_one) {
    net_trans_manage_t mgr = task->m_mgr;
    
    if (task->m_header == NULL) {
        task->m_header = curl_slist_append(NULL, header_one);

        if (task->m_header == NULL) {
            CPE_ERROR(
                mgr->m_em, "trans: %s-%d: append header %s create slist fail!",
                mgr->m_name, task->m_id, header_one);
            return -1;
        }

        if (curl_easy_setopt(task->m_handler, CURLOPT_HTTPHEADER, task->m_header) != CURLE_OK) {
            CPE_ERROR(
                mgr->m_em, "trans: %s-%d: append header %s set opt fail!", mgr->m_name, task->m_id, header_one);
            return -1;
        }
    }
    else {
        if (curl_slist_append(task->m_header, header_one) == NULL) {
            CPE_ERROR(mgr->m_em, "trans: %s-%d: append header %s add to slist fail!", mgr->m_name, task->m_id, header_one);
            return -1;
        }
    }

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: append header %s!", mgr->m_name, task->m_id, header_one);
    }

    return 0;
}

static const char * net_trans_task_dump(net_trans_manage_t mgr, char *i_ptr, size_t size, char nohex) {
    mem_buffer_t buffer = net_trans_manage_tmp_buffer(mgr);
    mem_buffer_clear_data(buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    stream_dump_data((write_stream_t)&ws, i_ptr, size, nohex);
    stream_putc((write_stream_t)&ws, 0);
    return (const char *)mem_buffer_make_continuous(buffer, 0);
}
 
static int net_trans_task_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp) {
    net_trans_task_t task = userp;
    net_trans_manage_t mgr = task->m_mgr;

    int64_t cur_ms = cur_time_ms();
    int64_t pre_ms = task->m_trace_last_time_ms == 0 ? task->m_trace_begin_time_ms : task->m_trace_last_time_ms;
    task->m_trace_last_time_ms = cur_ms;
    
    int32_t cost_from_begin = (int32_t)(cur_ms - task->m_trace_begin_time_ms);
    int32_t cost_from_pre = (int32_t)(cur_ms - pre_ms);

    switch(type) {
    case CURLINFO_TEXT: {
        char * end = data + size;
        char * p = cpe_str_trim_tail(end, data);
        char keep = 0;
        if (p < end) {
            keep = *p;
            *p = 0;
        }

        CPE_INFO(
            mgr->m_em, "trans: %s-%d: %0.5d(+%0.5d) == Info %s",
            mgr->m_name, task->m_id, cost_from_begin, cost_from_pre,
            data);
        
        if (p < end) {
            *p = keep;
        }

        break;
    }
    case CURLINFO_HEADER_OUT:
        CPE_INFO(
            mgr->m_em, "trans: %s-%d: %0.5d(+%0.5d) => header: %s",
            mgr->m_name, task->m_id, cost_from_begin, cost_from_pre,
            net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_DATA_OUT:
        CPE_INFO(
            mgr->m_em, "trans: %s-%d: %0.5d(+%0.5d) => data(%d): %s",
            mgr->m_name, task->m_id, cost_from_begin, cost_from_pre,
            (int)size, net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_SSL_DATA_OUT:
        CPE_INFO(
            mgr->m_em, "trans: %s-%d: %0.5d(+%0.5d) => SSL data(%d): %s",
            mgr->m_name, task->m_id, cost_from_begin, cost_from_pre,
            (int)size, net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_HEADER_IN:
        CPE_INFO(
            mgr->m_em, "trans: %s-%d: %0.5d(+%0.5d) <= header: %s",
            mgr->m_name, task->m_id, cost_from_begin, cost_from_pre,
            net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_DATA_IN:
        CPE_INFO(
            mgr->m_em, "trans: %s-%d: %0.5d(+%0.5d) <= data(%d): %s",
            mgr->m_name, task->m_id,cost_from_begin, cost_from_pre,
            (int)size, net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_SSL_DATA_IN:
        CPE_INFO(
            mgr->m_em, "trans: %s-%d: %0.5d(+%0.5d) <= SSL data(%d): %s",
            mgr->m_name, task->m_id, cost_from_begin, cost_from_pre,
            (int)size, net_trans_task_dump(mgr, data, size, 1));
        break;
    default:
        break;
    }
 
    return 0;
}
 
void net_trans_task_set_debug(net_trans_task_t task, uint8_t is_debug) {
    task->m_debug = is_debug;
}

static size_t net_trans_task_read_cb(void * dest, size_t size, size_t nmemb, void * userp) {
    net_trans_task_t task = userp;

    uint32_t sz = size * nmemb;
    switch(task->m_read_cb(task, task->m_read_ctx, dest, &sz)) {
    case net_trans_task_read_op_success:
        return sz;
    case net_trans_task_read_op_abort:
        return CURL_READFUNC_ABORT;
    case net_trans_task_read_op_pause:
        return CURL_READFUNC_PAUSE;
    }
}

void net_trans_task_set_body_provider(
    net_trans_task_t task,
    net_trans_task_read_op_t read_cb, void * read_ctx, void (*read_ctx_free)(void *))
{
    if (task->m_read_ctx_free) {
        task->m_read_ctx_free(task->m_read_ctx);
    }

    task->m_read_cb = read_cb;
    task->m_read_ctx = read_ctx;
    task->m_read_ctx_free = read_ctx_free;

    if (task->m_read_cb) {
        curl_easy_setopt(task->m_handler, CURLOPT_READFUNCTION, net_trans_task_read_cb);
        curl_easy_setopt(task->m_handler, CURLOPT_READDATA, task);
    }
    else {
        curl_easy_setopt(task->m_handler, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(task->m_handler, CURLOPT_READDATA, NULL);
    }
}

void net_trans_task_clear_callback(net_trans_task_t task) {
    if (task->m_ctx_free) {
        task->m_ctx_free(task->m_ctx);
    }

    task->m_commit_op = NULL;
    task->m_write_op = NULL;
    task->m_progress_op = NULL;
    task->m_head_op = NULL;
    task->m_ctx = NULL;
    task->m_ctx_free = NULL;
}

void net_trans_task_set_callback(
    net_trans_task_t task,
    net_trans_task_commit_op_t commit,
    net_trans_task_progress_op_t progress,
    net_trans_task_write_op_t write,
    net_trans_task_head_op_t head,
    void * ctx, void (*ctx_free)(void *))
{
    if (task->m_ctx_free) {
        task->m_ctx_free(task->m_ctx);
    }

    task->m_commit_op = commit;
    task->m_write_op = write;
    task->m_progress_op = progress;
    task->m_head_op = head;
    task->m_ctx = ctx;
    task->m_ctx_free = ctx_free;
}

int net_trans_task_set_body(net_trans_task_t task, void const * data, uint32_t data_size) {
    net_trans_manage_t mgr = task->m_mgr;

    if (curl_easy_setopt(task->m_handler, CURLOPT_POSTFIELDSIZE, (int)data_size) != CURLE_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_COPYPOSTFIELDS, data) != CURLE_OK)
    {
        CPE_ERROR(
            mgr->m_em, "trans: %s-%d: set post %d data fail!",
            mgr->m_name, task->m_id, (int)data_size);
        return -1;
    }

    return 0;
}

int net_trans_task_set_user_agent(net_trans_task_t task, const char * user_agent) {
    net_trans_manage_t mgr = task->m_mgr;

    if (curl_easy_setopt(task->m_handler, CURLOPT_USERAGENT, user_agent) != CURLE_OK) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: set user-agent %s fail!", mgr->m_name, task->m_id, user_agent);
        return -1;
    }

    return 0;
}

int net_trans_task_set_net_interface(net_trans_task_t task, const char * net_interface) {
    net_trans_manage_t mgr = task->m_mgr;

    if (curl_easy_setopt(task->m_handler, CURLOPT_INTERFACE, net_interface) != CURLE_OK) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: set net-interface %s fail!", mgr->m_name, task->m_id, net_interface);
        return -1;
    }

    return 0;
}

int net_trans_task_set_follow_location(net_trans_task_t task, uint8_t enable) {
    net_trans_manage_t mgr = task->m_mgr;

    if (curl_easy_setopt(task->m_handler, CURLOPT_FOLLOWLOCATION, (long)enable) != CURLE_OK) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: set follow-location fail!", mgr->m_name, task->m_id);
        return -1;
    }

    return 0;
}

int net_trans_task_set_proxy_http_1_1(net_trans_task_t task, net_address_t address) {
    net_trans_manage_t mgr = task->m_mgr;

    char address_buf[128];
    struct write_stream_mem ws = CPE_WRITE_STREAM_MEM_INITIALIZER(address_buf, sizeof(address_buf));
    net_address_print((write_stream_t)&ws, address);
    stream_putc((write_stream_t)&ws, 0);
    
    if (curl_easy_setopt(task->m_handler, CURLOPT_PROXY, address_buf) != CURLE_OK) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: set proxy %s fail!", mgr->m_name, task->m_id, address_buf);
        return -1;
    }

    if (curl_easy_setopt(task->m_handler, CURLOPT_PROXYTYPE, CURLPROXY_HTTP) != CURLE_OK) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: set proxy type http fail!", mgr->m_name, task->m_id);
        return -1;
    }
    
    return 0;
}

int net_trans_task_set_http_protocol(net_trans_task_t task, net_trans_http_version_t i_http_version) {
    net_trans_manage_t mgr = task->m_mgr;

    long http_version = CURL_HTTP_VERSION_NONE;
    switch(i_http_version) {
    case CURL_HTTP_VERSION_NONE:
        http_version = net_trans_http_version_none;
        break;
    case CURL_HTTP_VERSION_1_0:
        http_version = net_trans_http_version_1_0;
        break;
    case CURL_HTTP_VERSION_1_1:
        http_version = net_trans_http_version_1_1;
        break;
    case CURL_HTTP_VERSION_2_0:
        http_version = net_trans_http_version_2_0;
        break;
    case CURL_HTTP_VERSION_2TLS:
        http_version = net_trans_http_version_2tls;
        break;
    case CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE:
        http_version = net_trans_http_version_2_prior_knowledge;
        break;
    }
    
    if (curl_easy_setopt(task->m_handler, CURLOPT_HTTP_VERSION, http_version) != CURLE_OK) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: set http version fail!", mgr->m_name, task->m_id);
        return -1;
    }

    return 0;
}

int net_trans_task_set_protect_vpn(net_trans_task_t task, uint8_t protect_vpn) {
    task->m_cfg_protect_vpn = protect_vpn;
    return 0;
}

int net_trans_task_start(net_trans_task_t task) {
    net_trans_manage_t mgr = task->m_mgr;

    assert(!task->m_is_free);

    if (task->m_state == net_trans_task_working) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: can`t start in state %s!", mgr->m_name, task->m_id, net_trans_task_state_str(task->m_state));
        return -1;
    }

    net_local_ip_stack_t ip_stack = net_schedule_local_ip_stack(mgr->m_schedule);
    if (ip_stack == net_trans_task_error_none) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: start: ip stack non, can`t start!", mgr->m_name, task->m_id);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: current ip stack %s!", mgr->m_name, task->m_id, net_local_ip_stack_str(ip_stack));
    }
    
    if (task->m_state == net_trans_task_done) {
        CURL * new_handler = curl_easy_duphandle(task->m_handler);
        if (new_handler == NULL) {
            CPE_ERROR(mgr->m_em, "trans: %s-%d: duphandler fail!", mgr->m_name, task->m_id);
            return -1;
        }

        if (mgr->m_debug) {
            CPE_INFO(mgr->m_em, "trans: %s-%d: duphandler!", mgr->m_name, task->m_id);
        }

        curl_easy_cleanup(task->m_handler);
        task->m_handler = new_handler;
    }

    if (task->m_head_op) {
        curl_easy_setopt(task->m_handler, CURLOPT_HEADERFUNCTION, net_trans_task_header_cb);
        curl_easy_setopt(task->m_handler, CURLOPT_HEADERDATA, task);
    }

    if (task->m_debug) {
        task->m_trace_begin_time_ms = cur_time_ms();
        curl_easy_setopt(task->m_handler, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(task->m_handler, CURLOPT_DEBUGFUNCTION, net_trans_task_trace);
        curl_easy_setopt(task->m_handler, CURLOPT_DEBUGDATA, task);
        curl_easy_setopt(task->m_handler, CURLOPT_STDERR, stderr);
    }
    else {
        curl_easy_setopt(task->m_handler, CURLOPT_VERBOSE, 0L);
    }

    if (task->m_cfg_protect_vpn) {
        curl_easy_setopt(task->m_handler, CURLOPT_SOCKOPTFUNCTION, net_trans_task_sock_config_cb);
        curl_easy_setopt(task->m_handler, CURLOPT_SOCKOPTDATA, task);
    }

    if (net_address_type(task->m_target_address) == net_address_domain) {
        if (task->m_cfg_explicit_dns) {
            if (net_trans_task_setup_dns(mgr, task, ip_stack) != 0) return -1;
        }
    }

    CURLcode rc = curl_multi_add_handle(mgr->m_multi_handle, task->m_handler);
    if (rc != 0) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: start: curl_multi_add_handle error, rc=%d (%s)", mgr->m_name, task->m_id, rc, curl_easy_strerror(rc));
        return -1;
    }
    task->m_state = net_trans_task_working;

    mgr->m_still_running = 1;
    
    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: start!", mgr->m_name, task->m_id);
    }

    return 0;
}

#define _net_trans_task_cost_info_one(__entry, __tag)                   \
    buf = 0;                                                            \
    if ((rc = curl_easy_getinfo(task->m_handler, __tag, &buf)) != CURLE_OK) { \
        cost_info->__entry = -1;                                        \
        CPE_ERROR(                                                      \
            mgr->m_em, "trans: %s-%d: get cost info: get " #__tag  " fail, rc=%d (%s)", \
            mgr->m_name, task->m_id, rc, curl_easy_strerror(rc));       \
        rv = -1;                                                        \
    }                                                                   \
    else {                                                              \
        cost_info->__entry = (int32_t)(buf / 1000);                     \
    }

int net_trans_task_cost_info(net_trans_task_t task, net_trans_task_cost_info_t cost_info) {
    net_trans_manage_t mgr = task->m_mgr;
    CURLcode rc;
    curl_off_t buf;
    int rv = 0;

    _net_trans_task_cost_info_one(dns_ms, CURLINFO_NAMELOOKUP_TIME_T);
    _net_trans_task_cost_info_one(connect_ms, CURLINFO_CONNECT_TIME_T);
    _net_trans_task_cost_info_one(app_connect_ms, CURLINFO_APPCONNECT_TIME_T);
    _net_trans_task_cost_info_one(pre_transfer_ms, CURLINFO_PRETRANSFER_TIME_T);
    _net_trans_task_cost_info_one(start_transfer_ms, CURLINFO_STARTTRANSFER_TIME_T);
    _net_trans_task_cost_info_one(total_ms, CURLINFO_TOTAL_TIME_T);
    _net_trans_task_cost_info_one(redirect_ms, CURLINFO_REDIRECT_TIME_T);

    return rv;
}

const char * net_trans_task_state_str(net_trans_task_state_t state) {
    switch(state) {
    case net_trans_task_init:
        return "trans-task-init";
    case net_trans_task_working:
        return "trans-task-working";
    case net_trans_task_done:
        return "trans-task-done";
    }
}

const char * net_trans_task_result_str(net_trans_task_result_t result) {
    switch(result) {
    case net_trans_result_unknown:
        return "trans-task-result-unknown";
    case net_trans_result_complete:
        return "trans-task-result-complete";
    case net_trans_result_error:
        return "trans-task-result-error";
    case net_trans_result_cancel:
        return "trans-task-result-cancel";
    }
}

const char * net_trans_task_error_str(net_trans_task_error_t err) {
    switch(err) {
    case net_trans_task_error_none:
        return "none";
    case net_trans_task_error_dns_resolve_fail:
        return "dns-resolve-fail";
    case net_trans_task_error_timeout:
        return "timeout";
    case net_trans_task_error_remote_reset:
        return "remote-reset";
    case net_trans_task_error_net_unreachable:
        return "net-unreachable";
    case net_trans_task_error_net_down:
        return "net-down";
    case net_trans_task_error_host_unreachable:
        return "host-unreachable";
    case net_trans_task_error_connect:
        return "connect";
    case net_trans_task_error_local_cancel:
        return "local-cancel";
    case net_trans_task_error_internal:
        return "internal";
    }

    return "unknown";
}

int net_trans_task_error_from_str(const char * err, net_trans_task_error_t * r) {
    if (strcmp(err, "none") == 0) {
        *r = net_trans_task_error_none;
    }
    else if (strcmp(err, "dns-resolve-fail") == 0) {
        *r = net_trans_task_error_dns_resolve_fail;
    }
    else if (strcmp(err, "timeout") == 0) {
        *r = net_trans_task_error_timeout;
    }
    else if (strcmp(err, "remote-reset") == 0) {
        *r = net_trans_task_error_remote_reset;
    }
    else if (strcmp(err, "net-unreachable") == 0) {
        *r = net_trans_task_error_net_unreachable;
    }
    else if (strcmp(err, "net-down") == 0) {
        *r = net_trans_task_error_net_down;
    }
    else if (strcmp(err, "host-unreachable") == 0) {
        *r = net_trans_task_error_host_unreachable;
    }
    else if (strcmp(err, "connect") == 0) {
        *r = net_trans_task_error_connect;
    }
    else if (strcmp(err, "internal") == 0) {
        *r = net_trans_task_error_internal;
    }
    else {
        return -1;
    }

    return 0;
}

uint8_t net_trans_task_error_is_network_error(net_trans_task_error_t err) {
    switch(err) {
    case net_trans_task_error_dns_resolve_fail:
    case net_trans_task_error_remote_reset:
    case net_trans_task_error_net_unreachable:
    case net_trans_task_error_net_down:
    case net_trans_task_error_host_unreachable:
    case net_trans_task_error_connect:
        return 1;
    default:
        return 0;
    }
}

const char * net_trans_method_str(net_trans_method_t method) {
    switch(method) {
    case net_trans_method_get:
        return "get";
    case net_trans_method_post:
        return "post";
    case net_trans_method_put:
        return "put";
    case net_trans_method_delete:
        return "delete";
    case net_trans_method_patch:
        return "patch";
    }
}

uint32_t net_trans_task_hash(net_trans_task_t o, void * user_data) {
    return o->m_id;
}

int net_trans_task_eq(net_trans_task_t l, net_trans_task_t r, void * user_data) {
    return l->m_id == r->m_id;
}

static size_t net_trans_task_header_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
    net_trans_task_t task = stream;
    size_t total_length = size * nmemb;

    if (task->m_head_op == NULL) return total_length;
    if (task->m_is_free) return total_length;

    char * head_line = (char*)ptr;

    char * sep = strchr(head_line, ':');
    if (sep == NULL) return total_length;

    char * name_last = cpe_str_trim_tail(sep, head_line);
    char name_keep = *name_last;
    *name_last = 0;

    const char * value = cpe_str_trim_head(sep + 1);

    uint8_t tag_local = 0;
    if (task->m_in_callback == 0) {
        task->m_in_callback = 1;
        tag_local = 1;
    }
        
    task->m_head_op(task, task->m_ctx, head_line, value);

    if (tag_local) {
        task->m_in_callback = 0;

        if (task->m_is_free) {
            task->m_commit_op = NULL;
        }
    }

    *name_last = name_keep;

    return total_length;
}

static size_t net_trans_task_write_cb(char *ptr, size_t size, size_t nmemb, void * userdata) {
	net_trans_task_t task = userdata;
    net_trans_manage_t mgr = task->m_mgr;
	size_t total_length = size * nmemb;
    ssize_t write_size;

    if (task->m_is_free) return total_length;
    
    if (task->m_write_op) {
        uint8_t tag_local = 0;
        if (task->m_in_callback == 0) {
            task->m_in_callback = 1;
            tag_local = 1;
        }
        
        task->m_write_op(task, task->m_ctx, ptr, total_length);

        if (tag_local) {
            task->m_in_callback = 0;

            if (task->m_is_free) {
                task->m_commit_op = NULL;
            }
        }
    }
    else {
        write_size = mem_buffer_append(&task->m_buffer, ptr, total_length);
        if (write_size != (ssize_t)total_length) {
            CPE_ERROR(
                mgr->m_em, "trans: %s-%d): append %d data fail, return %d!",
                mgr->m_name, task->m_id, (int)total_length, (int)write_size);
        }
        else {
            if (mgr->m_debug) {
                CPE_INFO(mgr->m_em, "trans: %s-%d: receive %d data!", mgr->m_name, task->m_id, (int)total_length);
            }
        }
    }
    
    return total_length;
}

static int net_trans_task_prog_cb(void *p, double dltotal, double dlnow, double ult, double uln) {
    net_trans_task_t task = (net_trans_task_t)p;
    (void)ult;
    (void)uln;

    if (task->m_is_free) return 0;
    
    if (task->m_progress_op) {
        uint8_t tag_local = 0;
        if (task->m_in_callback == 0) {
            task->m_in_callback = 1;
            tag_local = 1;
        }
        
        task->m_progress_op(task, task->m_ctx, dltotal, dlnow);

        if (tag_local) {
            task->m_in_callback = 0;

            if (task->m_is_free) {
                task->m_commit_op = NULL;
            }
        }
    }
    
    return 0;
}

int net_trans_task_set_done(net_trans_task_t task, net_trans_task_result_t result, net_trans_task_error_t err, const char * err_addition) {
    net_trans_manage_t mgr = task->m_mgr;

    if (task->m_state != net_trans_task_working) {
        CPE_ERROR(
            mgr->m_em, "trans: %s-%d: can`t done in state %s!",
            mgr->m_name, task->m_id, net_trans_task_state_str(task->m_state));
        assert(0);
        return -1;
    }

    assert(task->m_state == net_trans_task_working);
    curl_multi_remove_handle(mgr->m_multi_handle, task->m_handler);

    task->m_result = result;
    task->m_error = err;
    task->m_state = net_trans_task_done;

    if (task->m_error_addition) {
        mem_free(mgr->m_alloc, task->m_error_addition);
        task->m_error_addition = NULL;
    }
    if (err_addition) {
        task->m_error_addition = cpe_str_mem_dup(mgr->m_alloc, err_addition);
    }

    if (mgr->m_debug) {
        struct net_trans_task_cost_info cost_info;
        bzero(&cost_info, sizeof(cost_info));
        net_trans_task_cost_info(task, &cost_info);

        if (result == net_trans_result_error) {
            CPE_INFO(
                mgr->m_em, "trans: %s-%d: done, result is %s, error=%s(%s)"
                " | cost: dns %.2f --> connect %.2f --> app-connect %.2f --> pre-transfer %.2f"
                " --> start-transfer %.2f --> total %.2f --> redirect %.2f",
                mgr->m_name, task->m_id, net_trans_task_result_str(task->m_result),
                net_trans_task_error_str(err), err_addition ? err_addition : "",
                (float)cost_info.dns_ms / 1000.0f,
                (float)cost_info.connect_ms / 1000.0f,
                (float)cost_info.app_connect_ms / 1000.0f,
                (float)cost_info.pre_transfer_ms / 1000.0f,
                (float)cost_info.start_transfer_ms / 1000.0f,
                (float)cost_info.total_ms / 1000.0f,
                (float)cost_info.redirect_ms / 1000.0f);
        }
        else {
            CPE_INFO(
                mgr->m_em, "trans: %s-%d: done, result is %s"
                " | cost: dns %.2f --> connect %.2f --> app-connect %.2f --> pre-transfer %.2f"
                " --> start-transfer %.2f --> total %.2f --> redirect %.2f",
                mgr->m_name, task->m_id, net_trans_task_result_str(task->m_result),
                (float)cost_info.dns_ms / 1000.0f,
                (float)cost_info.connect_ms / 1000.0f,
                (float)cost_info.app_connect_ms / 1000.0f,
                (float)cost_info.pre_transfer_ms / 1000.0f,
                (float)cost_info.start_transfer_ms / 1000.0f,
                (float)cost_info.total_ms / 1000.0f,
                (float)cost_info.redirect_ms / 1000.0f);
        }
    }

    if (task->m_commit_op) {
        net_trans_task_commit_op_t commit_op = task->m_commit_op;
        task->m_commit_op = NULL;

        uint8_t tag_local = 0;
        if (task->m_in_callback == 0) {
            task->m_in_callback = 1;
            tag_local = 1;
        }

        commit_op(task, task->m_ctx, mem_buffer_make_continuous(&task->m_buffer, 0), mem_buffer_size(&task->m_buffer));

        if (tag_local) {
            task->m_in_callback = 0;
            if (task->m_is_free) {
                net_trans_task_free(task);
            }
            else {
                assert(task->m_handler);
                curl_easy_cleanup(task->m_handler);
                task->m_handler = NULL;

                assert(task->m_watcher == NULL);
                /* if (task->m_watcher) { */
                /*     net_watcher_free(task->m_watcher); */
                /*     task->m_watcher = NULL; */
                /* } */
            }
        }
    }
    else {
        if (task->m_in_callback == 0) {
            net_trans_task_free(task);
        }
    }

    return 0;
}

static int net_trans_task_setup_url_origin(net_trans_manage_t mgr, net_trans_task_t task, const char * uri) {
    if (curl_easy_setopt(task->m_handler, CURLOPT_URL, uri) != (int)CURLE_OK) return -1;

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: url: %s", mgr->m_name, task->m_id, uri);
    }
    
    return 0;
}

static int net_trans_task_setup_url_ipv6(
    net_trans_manage_t mgr, net_trans_task_t task,
    const char * uri, const char * protocol, net_address_t host, const char * uri_left)
{
    net_address_t addr_ipv6 = net_address_create_ipv6_from_ipv4_nat(mgr->m_schedule, host);
    if (addr_ipv6 == NULL) {
        CPE_ERROR(
            mgr->m_em, "trans: %s-: setup-url: create ipv6 map addr from %s fail!",
            mgr->m_name, net_address_host(net_trans_manage_tmp_buffer(mgr), host));
        return -1;
    }

    mem_buffer_t tmp_buffer = net_trans_manage_tmp_buffer(mgr);
    mem_buffer_clear_data(tmp_buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(tmp_buffer);

    stream_printf((write_stream_t)&ws, "%s://[", protocol);
    net_address_print((write_stream_t)&ws, addr_ipv6);
    stream_printf((write_stream_t)&ws, "]%s", uri_left);
    stream_putc((write_stream_t)&ws, 0);
    
    net_address_free(addr_ipv6);

    const char * alert_uri = mem_buffer_make_continuous(tmp_buffer, 0);
    if (curl_easy_setopt(task->m_handler, CURLOPT_URL, alert_uri) != (int)CURLE_OK) return -1;

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: url: %s(%s)", mgr->m_name, task->m_id, alert_uri, uri);
    }

    return 0;
}

static int net_trans_task_setup_url_ipv4(
    net_trans_manage_t mgr, net_trans_task_t task,
    const char * uri, const char * protocol, net_address_t host, const char * uri_left)
{
    net_address_t addr_ipv4 = net_address_create_ipv4_from_ipv6_map(mgr->m_schedule, host);
    if (addr_ipv4 == NULL) {
        CPE_ERROR(
            mgr->m_em, "trans: %s-: setup-url: create ipv4 map addr from %s fail!",
            mgr->m_name, net_address_host(net_trans_manage_tmp_buffer(mgr), host));
        return -1;
    }

    mem_buffer_t tmp_buffer = net_trans_manage_tmp_buffer(mgr);
    mem_buffer_clear_data(tmp_buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(tmp_buffer);

    stream_printf((write_stream_t)&ws, "%s://", protocol);
    net_address_print((write_stream_t)&ws, addr_ipv4);
    stream_printf((write_stream_t)&ws, "%s", uri_left);
    stream_putc((write_stream_t)&ws, 0);
    
    net_address_free(addr_ipv4);

    const char * alert_uri = mem_buffer_make_continuous(tmp_buffer, 0);
    if (curl_easy_setopt(task->m_handler, CURLOPT_URL, alert_uri) != (int)CURLE_OK) return -1;

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: url: %s(%s)", mgr->m_name, task->m_id, alert_uri, uri);
    }

    return 0;
}

static int net_trans_task_setup_dns(net_trans_manage_t mgr, net_trans_task_t task, net_local_ip_stack_t ip_stack) {
    struct sockaddr_storage dnssevraddrs[10];
    uint8_t addr_count = CPE_ARRAY_SIZE(dnssevraddrs);
    if (getdnssvraddrs(dnssevraddrs, &addr_count, NULL) != 0) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: dns: get dns servers error!", mgr->m_name, task->m_id);
        return -1;
    }

    char dns_buf[512];
    size_t dns_buf_sz = 0;

    uint8_t i;
    for(i = 0; i < addr_count; ++i) {
        struct sockaddr_storage * sock_addr = &dnssevraddrs[i];
        if (sock_addr->ss_family == AF_INET6) {
            if (ip_stack != net_local_ip_stack_ipv6 && ip_stack != net_local_ip_stack_dual) {
                if (mgr->m_debug) {
                    CPE_INFO(
                        mgr->m_em, "trans: %s-%d: dns: protect ipv6 address for stack %s",
                        mgr->m_name, task->m_id, net_local_ip_stack_str(ip_stack));
                }
                continue;
            }
        }
        else if (sock_addr->ss_family == AF_INET) {
            if (ip_stack != net_local_ip_stack_ipv4 && ip_stack != net_local_ip_stack_dual) {
                if (mgr->m_debug) {
                    CPE_INFO(
                        mgr->m_em, "trans: %s-%d: dns: protect ipv4 address for stack %s",
                        mgr->m_name, task->m_id, net_local_ip_stack_str(ip_stack));
                }
                continue;
            }
        }
        else {
            if (mgr->m_debug) {
                CPE_INFO(
                    mgr->m_em, "trans: %s-%d: dns: protect address for not support family %d",
                    mgr->m_name, task->m_id, sock_addr->ss_family);
            }
            continue;
        }

        char one_buf[64];
        char * one_address = sock_get_addr(one_buf, sizeof(one_buf), sock_addr, sizeof(*sock_addr), 0, mgr->m_em);
        if (dns_buf_sz > 0) {
            dns_buf_sz += snprintf(dns_buf + dns_buf_sz, sizeof(dns_buf) - dns_buf_sz, ",%s", one_address);
        }
        else {
            dns_buf_sz += snprintf(dns_buf + dns_buf_sz, sizeof(dns_buf) - dns_buf_sz, "%s", one_address);
        }
    }

    dns_buf[dns_buf_sz] = 0;

    if (dns_buf_sz == 0) {
        CPE_ERROR(mgr->m_em, "trans: %s-%d: dns: no dns server!", mgr->m_name, task->m_id);
        return -1;
    }

    curl_easy_setopt(task->m_handler, CURLOPT_DNS_SERVERS, dns_buf);

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: %s-%d: dns: using %s!", mgr->m_name, task->m_id, dns_buf);
    }
    
    return 0;
}

static int net_trans_task_sock_config_cb(void * p, curl_socket_t curlfd, curlsocktype purpose) {
    net_trans_task_t task = (net_trans_task_t)p;
    //net_trans_manage_t mgr = task->m_mgr;

    if (purpose == CURLSOCKTYPE_IPCXN && task->m_cfg_protect_vpn) {
        /* if (sock_protect_vpn(curlfd, mgr->m_em) != 0) { */
        /*     CPE_INFO(mgr->m_em, "trans: %s-%d: protect vpn fail!", mgr->m_name, task->m_id); */
        /*     return -1; */
        /* } */
    }

    return 0;
}
