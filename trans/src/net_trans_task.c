#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
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
static int net_trans_task_init_dns(net_trans_manage_t mgr, net_trans_task_t task);

net_trans_task_t
net_trans_task_create(net_trans_manage_t mgr, net_trans_method_t method, const char * uri, uint8_t is_debug) {
    net_trans_task_t task = NULL;

    if (net_schedule_local_ip_stack(mgr->m_schedule) == net_local_ip_stack_none) {
        CPE_ERROR(mgr->m_em, "trans: task: ip stack non, can`t create task!");
        return NULL;
    }
    
    task = TAILQ_FIRST(&mgr->m_free_tasks);
    if (task) {
        TAILQ_REMOVE(&mgr->m_free_tasks, task, m_next_for_mgr);
    }
    else {
        task = mem_alloc(mgr->m_alloc, sizeof(struct net_trans_task));
        if (task == NULL) {
            CPE_ERROR(mgr->m_em, "trans: task: alloc fail");
            goto CREATED_ERROR;
        }
    }

    task->m_mgr = mgr;
    task->m_watcher = NULL;
    task->m_id = mgr->m_max_task_id + 1;
    task->m_state = net_trans_task_init;
    task->m_result = net_trans_result_unknown;
    task->m_error = net_trans_task_error_none;

    task->m_header = NULL;
    task->m_debug = 0;

    task->m_handler = curl_easy_init();
    if (task->m_handler == NULL) {
        CPE_ERROR(mgr->m_em, "trans: task: curl_easy_init fail!");
        goto CREATED_ERROR;
    }
    curl_easy_setopt(task->m_handler, CURLOPT_PRIVATE, task);

    curl_easy_setopt(task->m_handler, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(task->m_handler, CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt(task->m_handler, CURLOPT_NETRC, CURL_NETRC_IGNORED);
    
    task->m_in_callback = 0;
    task->m_is_free = 0;
    task->m_commit_op = NULL;
    task->m_write_op = NULL;
    task->m_head_op = NULL;
    task->m_progress_op = NULL;
    task->m_ctx = NULL;
    task->m_ctx_free = NULL;

    net_trans_task_set_debug(task, is_debug);
    
	curl_easy_setopt(task->m_handler, CURLOPT_WRITEFUNCTION, net_trans_task_write_cb);
	curl_easy_setopt(task->m_handler, CURLOPT_WRITEDATA, task);

    curl_easy_setopt(task->m_handler, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(task->m_handler, CURLOPT_PROGRESSFUNCTION, net_trans_task_prog_cb);
    curl_easy_setopt(task->m_handler, CURLOPT_PROGRESSDATA, task);

    curl_easy_setopt(task->m_handler, CURLOPT_URL, uri);
    
    switch(method) {
    case net_trans_method_get:
        break;
    case net_trans_method_post:
        curl_easy_setopt(task->m_handler, CURLOPT_POST, 1L);
        break;
    }

    if (cpe_str_start_with(uri, "https")) {
        curl_easy_setopt(task->m_handler, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(task->m_handler, CURLOPT_SSL_VERIFYHOST, 0);
    }

    mem_buffer_init(&task->m_buffer, mgr->m_alloc);

    if (net_trans_task_init_dns(mgr, task) != 0) goto CREATED_ERROR;

    cpe_hash_entry_init(&task->m_hh_for_mgr);
    if (cpe_hash_table_insert_unique(&mgr->m_tasks, task) != 0) {
        CPE_ERROR(mgr->m_em, "trans: task: id duplicate!");
        goto CREATED_ERROR;
    }

    mgr->m_max_task_id++;
    return task;

CREATED_ERROR:
    if (task) {
        TAILQ_INSERT_TAIL(&mgr->m_free_tasks, task, m_next_for_mgr);
        task = NULL;
    }
    
    return NULL;
}

void net_trans_task_free(net_trans_task_t task) {
    net_trans_manage_t mgr = task->m_mgr;

    if (task->m_in_callback) {
        task->m_state = net_trans_task_done;
        task->m_is_free = 1;
        return;
    }

    if (task->m_state != net_trans_task_done) {
        task->m_is_free = 1;
        net_trans_task_set_done(task, net_trans_result_cancel, net_trans_task_error_internal);
        return;
    }

    if (mgr->m_debug || task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: task %d: free!", task->m_id);
    }

    curl_easy_cleanup(task->m_handler);
    task->m_handler = NULL;

    if (task->m_watcher) {
        net_watcher_free(task->m_watcher);
        task->m_watcher = NULL;
    }
    
    if (task->m_ctx_free) {
        task->m_ctx_free(task->m_ctx);
        task->m_ctx = NULL;
        task->m_ctx_free = NULL;
    }

    mem_buffer_clear(&task->m_buffer);

    if (task->m_header) {
        curl_slist_free_all(task->m_header);
        task->m_header = NULL;
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

net_trans_task_state_t net_trans_task_state(net_trans_task_t task) {
    return task->m_state;
}

net_trans_task_result_t net_trans_task_result(net_trans_task_t task) {
    return task->m_result;
}

net_trans_task_error_t net_trans_task_error(net_trans_task_t task) {
    return task->m_error;
}

int16_t net_trans_task_res_code(net_trans_task_t task) {
    long http_code = 0;
    return curl_easy_getinfo(task->m_handler, CURLINFO_RESPONSE_CODE, &http_code) ==  CURLE_OK
        ? (int16_t)http_code
        : 0;
}

int net_trans_task_set_skip_data(net_trans_task_t task, ssize_t skip_length) {
    net_trans_manage_t mgr = task->m_mgr;

    if (curl_easy_setopt(task->m_handler, CURLOPT_RESUME_FROM_LARGE, skip_length > 0 ? (curl_off_t)skip_length : (curl_off_t)0) != (int)CURLM_OK) {
        CPE_ERROR(
            mgr->m_em, "trans: task %d: set skip data %d error!",
            task->m_id, (int)skip_length);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: task %d: set skip data %d!", task->m_id, (int)skip_length);
    }

    return 0;
}

int net_trans_task_set_timeout_ms(net_trans_task_t task, uint64_t timeout_ms) {
    net_trans_manage_t mgr = task->m_mgr;

	if (curl_easy_setopt(task->m_handler, CURLOPT_TIMEOUT_MS, timeout_ms) != (int)CURLM_OK) {
        CPE_ERROR(
            mgr->m_em, "trans: task %d: set timeout " FMT_UINT64_T "ms error!",
            task->m_id, timeout_ms);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(
            mgr->m_em, "trans: task %d: set timeout " FMT_UINT64_T "ms!",
            task->m_id, timeout_ms);
    }

    return 0;
}

int net_trans_task_set_connection_timeout_ms(net_trans_task_t task, uint64_t timeout_ms) {
    net_trans_manage_t mgr = task->m_mgr;

	if (curl_easy_setopt(task->m_handler, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms) != (int)CURLM_OK) {
        CPE_ERROR(
            mgr->m_em, "trans: task %d: set connection-timeout " FMT_UINT64_T "ms error!",
            task->m_id, timeout_ms);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(
            mgr->m_em, "trans: task %d: set connection-timeout " FMT_UINT64_T "ms!",
            task->m_id, timeout_ms);
    }

    return 0;
}

int net_trans_task_set_useragent(net_trans_task_t task, const char * agent) {
    net_trans_manage_t mgr = task->m_mgr;

	if (curl_easy_setopt(task->m_handler, CURLOPT_USERAGENT, agent) != (int)CURLM_OK) {
        CPE_ERROR(
            mgr->m_em, "trans: task %d: set useragent %s error!",
            task->m_id, agent);
        return -1;
    }

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: task %d: set useragent %s!", task->m_id, agent);
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
                mgr->m_em, "trans: task %d: append header %s create slist fail!",
                task->m_id, header_one);
            return -1;
        }

        if (curl_easy_setopt(task->m_handler, CURLOPT_HTTPHEADER, task->m_header) != (int)CURLM_OK) {
            CPE_ERROR(
                mgr->m_em, "trans: task %d: append header %s set opt fail!", task->m_id, header_one);
            return -1;
        }
    }
    else {
        if (curl_slist_append(task->m_header, header_one) == NULL) {
            CPE_ERROR(mgr->m_em, "trans: task %d: append header %s add to slist fail!", task->m_id, header_one);
            return -1;
        }
    }

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: task %d: append header %s!", task->m_id, header_one);
    }

    return 0;
}

static const char * net_trans_task_dump(net_trans_manage_t mgr, char *i_ptr, size_t size, char nohex) {
    unsigned char * ptr = (unsigned char *)i_ptr;
    size_t i;
    size_t c;

    mem_buffer_t buffer = net_trans_manage_tmp_buffer(mgr);
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
 
static int net_trans_task_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp) {
    net_trans_task_t task = userp;
    net_trans_manage_t mgr = task->m_mgr;

    switch(type) {
    case CURLINFO_TEXT: {
        char * end = data + size;
        char * p = cpe_str_trim_tail(end, data);
        char keep = 0;
        if (p < end) {
            keep = *p;
            *p = 0;
        }

        CPE_INFO(mgr->m_em, "trans: task %d: == Info %s", task->m_id, data);
        
        if (p < end) {
            *p = keep;
        }

        break;
    }
    case CURLINFO_HEADER_OUT:
        CPE_INFO(mgr->m_em, "trans: task %d: => header: %s", task->m_id, net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_DATA_OUT:
        CPE_INFO(
            mgr->m_em, "trans: task %d: => data(%d): %s",
            task->m_id, (int)size, net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_SSL_DATA_OUT:
        CPE_INFO(
            mgr->m_em, "trans: task %d: => SSL data(%d): %s",
            task->m_id, (int)size, net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_HEADER_IN:
        CPE_INFO(mgr->m_em, "trans: task %d: <= header: %s", task->m_id, net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_DATA_IN:
        CPE_INFO(
            mgr->m_em, "trans: task %d: <= data(%d): %s",
            task->m_id, (int)size, net_trans_task_dump(mgr, data, size, 1));
        break;
    case CURLINFO_SSL_DATA_IN:
        CPE_INFO(
            mgr->m_em, "trans: task %d: <= SSL data(%d): %s",
            task->m_id, (int)size, net_trans_task_dump(mgr, data, size, 1));
        break;
    default:
        break;
    }
 
    return 0;
}
 
void net_trans_task_set_debug(net_trans_task_t task, uint8_t is_debug) {
    task->m_debug = is_debug;
    if (is_debug) {
        curl_easy_setopt(task->m_handler, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(task->m_handler, CURLOPT_DEBUGFUNCTION, net_trans_task_trace);
        curl_easy_setopt(task->m_handler, CURLOPT_DEBUGDATA, task);
        curl_easy_setopt(task->m_handler, CURLOPT_STDERR, stderr);
    }
    else {
        curl_easy_setopt(task->m_handler, CURLOPT_VERBOSE, 0L);
    }
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

    if (curl_easy_setopt(task->m_handler, CURLOPT_POSTFIELDSIZE, (int)data_size) != (int)CURLM_OK
        || curl_easy_setopt(task->m_handler, CURLOPT_COPYPOSTFIELDS, data) != (int)CURLM_OK)
    {
        CPE_ERROR(
            mgr->m_em, "trans: task %d: set post %d data fail!",
            task->m_id, (int)data_size);
        return -1;
    }

    return 0;
}

int net_trans_task_set_user_agent(net_trans_task_t task, const char * user_agent) {
    net_trans_manage_t mgr = task->m_mgr;

    if (curl_easy_setopt(task->m_handler, CURLOPT_USERAGENT, user_agent) != (int)CURLM_OK) {
        CPE_ERROR(mgr->m_em, "trans: task %d: set user-agent %s fail!", task->m_id, user_agent);
        return -1;
    }

    return 0;
}

int net_trans_task_set_net_interface(net_trans_task_t task, const char * net_interface) {
    net_trans_manage_t mgr = task->m_mgr;

    if (curl_easy_setopt(task->m_handler, CURLOPT_INTERFACE, net_interface) != (int)CURLM_OK) {
        CPE_ERROR(mgr->m_em, "trans: task %d: set net-interface %s fail!", task->m_id, net_interface);
        return -1;
    }

    return 0;
}

int net_trans_task_start(net_trans_task_t task) {
    net_trans_manage_t mgr = task->m_mgr;
    int rc;

    assert(!task->m_is_free);

    if (task->m_state == net_trans_task_working) {
        CPE_ERROR(mgr->m_em, "trans: task %d: can`t start in state %s!", task->m_id, net_trans_task_state_str(task->m_state));
        return -1;
    }

    if (task->m_state == net_trans_task_done) {
        CURL * new_handler = curl_easy_duphandle(task->m_handler);
        if (new_handler == NULL) {
            CPE_ERROR(mgr->m_em, "trans: task %d: duphandler fail!", task->m_id);
            return -1;
        }

        if (mgr->m_debug) {
            CPE_INFO(mgr->m_em, "trans: task %d: duphandler!", task->m_id);
        }

        curl_easy_cleanup(task->m_handler);
        task->m_handler = new_handler;
    }

    if (task->m_head_op) {
        curl_easy_setopt(task->m_handler, CURLOPT_HEADERFUNCTION, net_trans_task_header_cb);
        curl_easy_setopt(task->m_handler, CURLOPT_HEADERDATA, task);
    }

    rc = curl_multi_add_handle(mgr->m_multi_handle, task->m_handler);
    if (rc != 0) {
        CPE_ERROR(mgr->m_em, "trans: task %d: curl_multi_add_handle error, rc=%d", task->m_id, rc);
        return -1;
    }
    task->m_state = net_trans_task_working;

    mgr->m_still_running = 1;

    if (mgr->m_debug) {
        CPE_INFO(mgr->m_em, "trans: task %d: start!", task->m_id);
    }

    return 0;
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
    case net_trans_task_error_connect:
        return "connect";
    case net_trans_task_error_internal:
        return "internal";
    }

    return "unknown";
}

uint32_t net_trans_task_hash(net_trans_task_t o, void * user_data) {
    return o->m_id;
}

int net_trans_task_eq(net_trans_task_t l, net_trans_task_t r, void * user_data) {
    return l->m_id == r->m_id;
}

static size_t net_trans_task_header_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
    char * head_line = (char*)ptr;
    size_t total_length = size * nmemb;

    char * sep = strchr(ptr, ':');
    if (sep == NULL) return total_length;

    net_trans_task_t task = stream;
    net_trans_manage_t mgr = task->m_mgr;
    
    char * name_last = cpe_str_trim_tail(sep, head_line);
    char name_keep = *name_last;
    *name_last = 0;

    const char * value = cpe_str_trim_head(sep + 1);

    if (task->m_debug) {
        CPE_INFO(
            mgr->m_em, "trans: task %d: head: %s=%s",
            task->m_id, head_line, value);
    }

    if (task->m_head_op) {
        uint8_t tag_local = 0;
        if (task->m_in_callback == 0) {
            task->m_in_callback = 1;
            tag_local = 1;
        }
        
        task->m_head_op(task, task->m_ctx, head_line, value);

        if (tag_local) {
            task->m_in_callback = 0;
        }
    }

    *name_last = name_keep;

    if (!task->m_in_callback && task->m_is_free) {
        task->m_is_free = 0;
        net_trans_task_free(task);
    }

    return total_length;
}

static size_t net_trans_task_write_cb(char *ptr, size_t size, size_t nmemb, void * userdata) {
	net_trans_task_t task = userdata;
    net_trans_manage_t mgr = task->m_mgr;
	size_t total_length = size * nmemb;
    ssize_t write_size;

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
                task->m_is_free = 0;
                net_trans_task_free(task);
            }
        }
    }
    else {
        write_size = mem_buffer_append(&task->m_buffer, ptr, total_length);
        if (write_size != (ssize_t)total_length) {
            CPE_ERROR(
                mgr->m_em, "trans: task %d): append %d data fail, return %d!",
                task->m_id, (int)total_length, (int)write_size);
        }
        else {
            if (mgr->m_debug) {
                CPE_INFO(mgr->m_em, "trans: task %d: receive %d data!", task->m_id, (int)total_length);
            }
        }
    }
    
    return total_length;
}

static int net_trans_task_prog_cb(void *p, double dltotal, double dlnow, double ult, double uln) {
    net_trans_task_t task = (net_trans_task_t)p;
    (void)ult;
    (void)uln;

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
                task->m_is_free = 0;
                net_trans_task_free(task);
            }
        }
    }
    
    return 0;
}

int net_trans_task_set_done(net_trans_task_t task, net_trans_task_result_t result, net_trans_task_error_t err) {
    net_trans_manage_t mgr = task->m_mgr;

    if (task->m_state != net_trans_task_working) {
        CPE_ERROR(
            mgr->m_em, "trans: task %d: can`t done in state %s!",
            task->m_id, net_trans_task_state_str(task->m_state));
        return -1;
    }

    assert(task->m_state == net_trans_task_working);
    curl_multi_remove_handle(mgr->m_multi_handle, task->m_handler);

    task->m_result = result;
    task->m_error = err;
    task->m_state = net_trans_task_done;

    if (mgr->m_debug) {
        if (result == net_trans_result_error) {
            CPE_INFO(
                mgr->m_em, "trans: task %d: done, result is %s, error=%s!",
                task->m_id, net_trans_task_result_str(task->m_result), net_trans_task_error_str(err));
        }
        else {
            CPE_INFO(
                mgr->m_em, "trans: task %d: done, result is %s!",
                task->m_id, net_trans_task_result_str(task->m_result));
        }
    }

    if (task->m_commit_op) {
        uint8_t tag_local = 0;
        if (task->m_in_callback == 0) {
            task->m_in_callback = 1;
            tag_local = 1;
        }

        task->m_commit_op(task, task->m_ctx, mem_buffer_make_continuous(&task->m_buffer, 0), mem_buffer_size(&task->m_buffer));

        if (tag_local) {
            task->m_in_callback = 0;

            if (task->m_is_free || task->m_state == net_trans_task_done) {
                task->m_is_free = 0;
                net_trans_task_free(task);
            }
        }
    }
    else {
        net_trans_task_free(task);
    }

    return 0;
}

static int net_trans_task_init_dns(net_trans_manage_t mgr, net_trans_task_t task) {
    struct sockaddr_storage dnssevraddrs[10];
    uint8_t addr_count = CPE_ARRAY_SIZE(dnssevraddrs);
    if (getdnssvraddrs(dnssevraddrs, &addr_count, mgr->m_em) != 0) {
        CPE_ERROR(mgr->m_em, "trans: task %d: dns: get dns servers error!", task->m_id);
        return -1;
    }

    char dns_buf[512];
    size_t dns_buf_sz = 0;

    net_local_ip_stack_t ip_stack = net_schedule_local_ip_stack(mgr->m_schedule);

    uint8_t i;
    for(i = 0; i < addr_count; ++i) {
        struct sockaddr_storage * sock_addr = &dnssevraddrs[i];
        if (sock_addr->ss_family == AF_INET6) {
            if (ip_stack != net_local_ip_stack_ipv6 && ip_stack != net_local_ip_stack_dual) {
                if (mgr->m_debug) {
                    CPE_INFO(
                        mgr->m_em, "trans: task %d: dns: ignore ipv4 address for stack %s",
                        task->m_id, net_local_ip_stack_str(ip_stack));
                }
                continue;
            }
        }
        else if (sock_addr->ss_family == AF_INET) {
            if (ip_stack != net_local_ip_stack_ipv4 && ip_stack != net_local_ip_stack_dual) {
                if (mgr->m_debug) {
                    CPE_INFO(
                        mgr->m_em, "trans: task %d: dns: ignore ipv4 address for stack %s",
                        task->m_id, net_local_ip_stack_str(ip_stack));
                }
                continue;
            }
        }
        else {
            if (mgr->m_debug) {
                CPE_INFO(
                    mgr->m_em, "trans: task %d: dns: ignore address for not support family %d",
                    task->m_id, sock_addr->ss_family);
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
        CPE_ERROR(mgr->m_em, "trans: task %d: dns: no dns server!", task->m_id);
        return -1;
    }

    curl_easy_setopt(task->m_handler, CURLOPT_DNS_SERVERS, dns_buf);

    if (task->m_debug) {
        CPE_INFO(mgr->m_em, "trans: task %d: dns: using %s!", task->m_id, dns_buf);
    }
    
    return 0;
}
