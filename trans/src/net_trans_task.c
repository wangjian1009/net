#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_trans_task_i.h"

static net_address_t net_trans_task_parse_address(
    net_trans_manage_t mgr, const char * url, uint8_t * is_https, const char * * relative_url);
static size_t net_trans_task_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata);
static int net_tranks_task_prog_cb(void *p, double dltotal, double dlnow, double ult, double uln);
        
net_trans_task_t
net_trans_task_create(net_trans_manage_t mgr, net_trans_method_t method, const char * uri) {
    uint8_t is_https;
    const char * relative_url;
    net_address_t address = net_trans_task_parse_address(mgr, uri, &is_https, &relative_url);
    if (address == NULL) return NULL;

    net_trans_task_t task = net_trans_task_create_relative(mgr, method, address, is_https, relative_url);
    
    net_address_free(address);

    return task;
}

net_trans_task_t net_trans_task_create_relative(
    net_trans_manage_t mgr, net_trans_method_t method,
    net_address_t address, uint8_t is_https, const char * relative_url)
{
    net_trans_task_t task = NULL;

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

    task->m_handler = curl_easy_init();
    if (task->m_handler == NULL) {
        CPE_ERROR(mgr->m_em, "trans: task: curl_easy_init fail!");
        goto CREATED_ERROR;
    }
    curl_easy_setopt(task->m_handler, CURLOPT_PRIVATE, task);

    curl_easy_setopt(task->m_handler, CURLOPT_NOSIGNAL, 1);
    
    task->m_in_callback = 0;
    task->m_is_free = 0;
    task->m_commit_op = NULL;
    task->m_write_op = NULL;
    task->m_progress_op = NULL;
    task->m_ctx = NULL;
    task->m_ctx_free = NULL;

	curl_easy_setopt(task->m_handler, CURLOPT_WRITEFUNCTION, net_trans_task_write_cb);
	curl_easy_setopt(task->m_handler, CURLOPT_WRITEDATA, task);

    curl_easy_setopt(task->m_handler, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(task->m_handler, CURLOPT_PROGRESSFUNCTION, net_tranks_task_prog_cb);
    curl_easy_setopt(task->m_handler, CURLOPT_PROGRESSDATA, task);

    switch(method) {
    case net_trans_method_get:
        break;
    case net_trans_method_post:
        curl_easy_setopt(task->m_handler, CURLOPT_POST, 1L);
        break;
    }
    curl_easy_setopt(task->m_handler, CURLOPT_URL, relative_url);

    /*     if (curl_easy_setopt(task->m_handler, CURLOPT_POST, 1L) != (int)CURLM_OK */
    /*     || curl_easy_setopt(task->m_handler, CURLOPT_POSTFIELDSIZE, (int)data_len) != (int)CURLM_OK */
    /*     || curl_easy_setopt(task->m_handler, CURLOPT_COPYPOSTFIELDS, data) != (int)CURLM_OK */
    /* { */
    /*     CPE_ERROR( */
    /*         mgr->m_em, "%s: task %d (%s): set post %d data to %s fail!", */
    /*         net_trans_manage_name(mgr), task->m_id, task->m_group->m_name, (int)data_len, uri); */
    /*     return -1; */
    /* } */

    if (is_https) {
        curl_easy_setopt(task->m_handler, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(task->m_handler, CURLOPT_SSL_VERIFYHOST, 0);
    }

    mem_buffer_init(&task->m_buffer, mgr->m_alloc);
    
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
    if (task->m_in_callback) {
        task->m_is_free = 1;
        return;
    }

    /* if (task->m_http_req) { */
    /*     net_http_req_free(task->m_http_req); */
    /*     task->m_http_req = NULL; */
    /* } */
    
    if (task->m_ctx_free) {
        task->m_ctx_free(task->m_ctx);
        task->m_ctx = NULL;
        task->m_ctx_free = NULL;
    }

    mem_buffer_clear(&task->m_buffer);
    
    net_trans_manage_t mgr = task->m_mgr;
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
    //TODO:
    return net_trans_result_unknown;
}

int16_t net_trans_task_res_code(net_trans_task_t task) {
    //TODO:
    return 200;
}

void net_trans_task_set_debug(net_trans_task_t task, uint8_t is_debug) {
    if (is_debug) {
        curl_easy_setopt(task->m_handler, CURLOPT_STDERR, stderr );
        curl_easy_setopt(task->m_handler, CURLOPT_VERBOSE, 1L);
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
    void * ctx, void (*ctx_free)(void *))
{
    if (task->m_ctx_free) {
        task->m_ctx_free(task->m_ctx);
    }

    task->m_commit_op = commit;
    task->m_write_op = write;
    task->m_progress_op = progress;
    task->m_ctx = ctx;
    task->m_ctx_free = ctx_free;
}

int net_trans_task_set_timeout(net_trans_task_t task, uint64_t timeout_ms) {
    /* if (task->m_http_req) { */
    /*     return net_http_req_start_timeout(task->m_http_req, (uint32_t)timeout_ms); */
    /* } */
    /* else { */
    /*     return -1; */
    /* } */
    return 0;
}

int net_trans_task_append_header(net_trans_task_t task, const char * name, const char * value) {
    /* if (net_http_req_write_head_pair(task->m_http_req, name, value) != 0) return -1; */

    /* if (strcasecmp(name, "Connection") == 0) { */
    /*     task->m_keep_alive = */
    /*         net_endpoint_connection_type(net_http_req_ep(task->m_http_req)) == net_http_connection_type_keep_alive ? 1 : 0; */
    /* } */
    
    return 0;
}

int net_trans_task_set_body(net_trans_task_t task, void const * data, uint32_t data_size) {
    //if (net_http_req_write_body_full(task->m_http_req, data, data_size) != 0) return -1;
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
    case net_trans_result_timeout:
        return "trans-task-result-timeout";
    case net_trans_result_cancel:
        return "trans-task-result-cancel";
    }
}

uint32_t net_trans_task_hash(net_trans_task_t o, void * user_data) {
    return o->m_id;
}

int net_trans_task_eq(net_trans_task_t l, net_trans_task_t r, void * user_data) {
    return l->m_id == r->m_id;
}

static net_address_t net_trans_task_parse_address(
    net_trans_manage_t mgr, const char * url, uint8_t * is_https, const char * * relative_url)
{
    const char * host_begin;
    if (cpe_str_start_with(url, "http://")) {
        *is_https = 0;
        host_begin = url + 7;
    }
    else if (cpe_str_start_with(url, "https://")) {
        *is_https = 1;
        host_begin = url + 8;
    }
    else {
        CPE_ERROR(mgr->m_em, "trans: url %s not support, no http or https protocol!", url);
        return NULL;
    }

    net_address_t address = NULL;

    const char * host_end = strchr(host_begin, '/');
    if (host_end) {
        char address_buf[128];
        size_t len = host_end - host_begin;
        if (len >= CPE_ARRAY_SIZE(address_buf)) {
            CPE_ERROR(mgr->m_em, "trans: url %s format error, length %d overflow!", url, (int)len);
            return NULL;
        }

        memcpy(address_buf, host_begin, len);
        address_buf[len] = 0;
        address = net_address_create_auto(mgr->m_schedule, address_buf);
        if (address == NULL) {
            CPE_ERROR(mgr->m_em, "trans: url %s format error, address=%s!", url, address_buf);
            return NULL;
        }

        *relative_url = host_end;
    }
    else {
        address = net_address_create_auto(mgr->m_schedule, host_begin);
        if (address == NULL) {
            CPE_ERROR(mgr->m_em, "trans: url %s format error, address=%s!", url, host_begin);
            return NULL;
        }
        
        *relative_url = "/";
    }

    if (net_address_port(address) == 0) {
        net_address_set_port(address, *is_https ? 443 : 80);
    }
    
    return address;
}

static size_t net_trans_task_write_cb(char *ptr, size_t size, size_t nmemb, void * userdata) {
	net_trans_task_t task = userdata;
    net_trans_manage_t mgr = task->m_mgr;
	size_t total_length = size * nmemb;
    ssize_t write_size;

    /* if (task->m_write_op) { */
    /*     task->m_in_callback = 1; */
    /*     task->m_write_op(task, task->m_write_ctx, ptr, total_length); */
    /*     task->m_in_callback = 0; */

    /*     if (task->m_is_free) { */
    /*         task->m_is_free = 0; */
    /*         net_trans_task_free(task); */
    /*     } */

    /*     return total_length; */
    /* } */
    
    return total_length;
}

static int net_tranks_task_prog_cb(void *p, double dltotal, double dlnow, double ult, double uln) {
    net_trans_task_t task = (net_trans_task_t)p;
    (void)ult;
    (void)uln;

    /* if (task->m_progress_op) { */
    /*     task->m_in_callback = 1; */
    /*     task->m_progress_op(task, task->m_progress_ctx, dltotal, dlnow); */
    /*     task->m_in_callback = 0; */

    /*     if (task->m_is_free) { */
    /*         task->m_is_free = 0; */
    /*         net_trans_task_free(task); */
    /*     } */
    /* } */
    
    return 0;
}

int net_trans_task_set_done(net_trans_task_t task, net_trans_task_result_t result, int err) {
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
    //task->m_errno = (net_trans_errno_t)err;
    task->m_state = net_trans_task_done;

    if (mgr->m_debug) {
        CPE_INFO(
            mgr->m_em, "trans: task %d: done, result is %s!",
            task->m_id, net_trans_task_result_str(task->m_result));
    }

    if (task->m_commit_op) {
        task->m_in_callback = 1;
        task->m_commit_op(task, task->m_ctx, mem_buffer_make_continuous(&task->m_buffer, 0), mem_buffer_size(&task->m_buffer));
        task->m_in_callback = 0;

        if (task->m_is_free || task->m_state == net_trans_task_done) {
            task->m_is_free = 0;
            net_trans_task_free(task);
        }
    }
    else {
        net_trans_task_free(task);
    }

    return 0;
}

/* static net_http_res_op_result_t net_trans_task_on_head(void * ctx, net_http_req_t req, const char * name, const char * value) { */
/*     net_trans_task_t task = ctx; */

/*     if (task->m_res_code == 0) { */
/*         task->m_res_code = net_http_req_res_code(req); */
/*         cpe_str_dup(task->m_res_message, sizeof(task->m_res_message), net_http_req_res_message(req)); */
/*     } */

/*     return net_http_res_op_success; */
/* } */

/* static net_http_res_op_result_t net_trans_task_on_body(void * ctx, net_http_req_t req, void * data, size_t data_size) { */
/*     net_trans_task_t task = ctx; */

/*     if (task->m_write_op) { */
/*         task->m_in_callback = 1; */
/*         task->m_write_op(task, task->m_ctx, data, data_size); */
/*         task->m_in_callback = 0; */

/*         if (task->m_is_free) { */
/*             net_trans_task_free(task); */
/*             return net_http_res_op_ignore; */
/*         } */
/*     } */
    
/*     return net_http_res_op_success; */
/* } */

/* static net_http_res_op_result_t net_trans_task_on_complete(void * ctx, net_http_req_t req, net_http_res_result_t result) { */
/*     net_trans_task_t task = ctx; */
/*     net_trans_manage_t mgr = task->m_mgr; */

/*     switch(result) { */
/*     case net_http_res_complete: */
/*         task->m_result = net_trans_result_complete; */
/*         task->m_res_code = net_http_req_res_code(req); */
/*         break; */
/*     case net_http_res_timeout: */
/*         task->m_result = net_trans_result_timeout; */
/*         break; */
/*     case net_http_res_canceled: */
/*         task->m_result = net_trans_result_cancel; */
/*         break; */
/*     case net_http_res_disconnected: */
/*         task->m_result = net_trans_result_cancel; */
/*         break; */
/*     } */

/*     assert(task->m_ep); */
/*     TAILQ_REMOVE(&task->m_ep->m_tasks, task, m_next_for_ep); */
/*     task->m_ep = NULL; */

/*     assert(task->m_http_req == req); */
/*     task->m_http_req = NULL; */
    
/*     if (task->m_commit_op) { */
/*         uint32_t buf_sz = 0; */
/*         void * buf = NULL; */
        
/*         if (task->m_write_op == NULL) { */
/*             net_endpoint_t base_endpoint = net_endpoint_net_ep(net_http_req_ep(req)); */
/*             buf_sz = net_endpoint_buf_size(base_endpoint, net_ep_buf_http_body); */

/*             if (net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_http_body, buf_sz, &buf) != 0) { */
/*                 CPE_ERROR(mgr->m_em, "trans: task %d: complete: get http data fail, size=%d!", task->m_id, buf_sz); */
/*                 task->m_result = net_trans_result_cancel; */
/*                 buf = NULL; */
/*                 buf_sz = 0; */
/*             } */
/*         } */

/*         task->m_in_callback = 1; */
/*         task->m_commit_op(task, task->m_ctx, buf, buf_sz); */
/*         task->m_in_callback = 0; */

/*         if (task->m_is_free) { */
/*             net_trans_task_free(task); */
/*             return net_http_res_op_ignore; */
/*         } */
/*     } */

/*     return net_http_res_op_success; */
/* } */
