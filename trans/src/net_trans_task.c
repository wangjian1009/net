#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_http_req.h"
#include "net_http_endpoint.h"
#include "net_trans_task_i.h"
#include "net_trans_host_i.h"
#include "net_trans_http_endpoint_i.h"

static net_address_t net_trans_task_parse_address(
    net_trans_manage_t mgr, const char * url, uint8_t * is_https, const char * * relative_url);
static net_http_res_op_result_t net_trans_task_on_body(void * ctx, net_http_req_t req, void * data, size_t data_size);
static net_http_res_op_result_t net_trans_task_on_complete(void * ctx, net_http_req_t req, net_http_res_result_t result);

net_trans_task_t
net_trans_task_create(net_trans_manage_t mgr, net_trans_method_t method, const char * uri) {
    net_address_t address = NULL;
    net_trans_host_t host = NULL;
    net_trans_http_endpoint_t http_ep = NULL;
    net_trans_task_t task = NULL;

    uint8_t is_https;
    const char * relative_url;
    address = net_trans_task_parse_address(mgr, uri, &is_https, &relative_url);
    if (address == NULL) return NULL;

    host = net_trans_host_check_create(mgr, address);
    if (host == NULL) goto CREATED_ERROR;
    net_address_free(address);
    address = NULL;

    http_ep = net_trans_host_alloc_endpoint(host);
    if (http_ep == NULL) goto CREATED_ERROR;

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
    task->m_ep = http_ep;
    task->m_id = mgr->m_max_task_id + 1;

    task->m_result = net_trans_result_unknown;
    task->m_res_code = 0;
    task->m_res_mine[0] = 0;
    task->m_res_charset[0] = 0;
    
    task->m_in_callback = 0;
    task->m_is_free = 0;
    task->m_commit_op = NULL;
    task->m_write_op = NULL;
    task->m_progress_op = NULL;
    task->m_ctx = NULL;
    task->m_ctx_free = NULL;

    task->m_http_req = net_http_req_create(
        net_http_endpoint_from_data(http_ep),
        method == net_trans_method_get ? net_http_req_method_get : net_http_req_method_post, relative_url);
    if (task->m_http_req == NULL) {
        CPE_ERROR(mgr->m_em, "trans: task: create http req fail!");
        goto CREATED_ERROR;
    }

    cpe_hash_entry_init(&task->m_hh_for_mgr);
    if (cpe_hash_table_insert_unique(&mgr->m_tasks, task) != 0) {
        CPE_ERROR(mgr->m_em, "trans: task: id duplicate!");
        goto CREATED_ERROR;
    }
    TAILQ_INSERT_TAIL(&task->m_ep->m_tasks, task, m_next_for_ep);

    mgr->m_max_task_id++;
    return task;

CREATED_ERROR:
    if (task) {
        if (task->m_http_req) {
            net_http_req_free(task->m_http_req);
            task->m_http_req = NULL;

            net_trans_http_endpoint_free(http_ep);
            http_ep = NULL;
        }
        
        task->m_ep = (net_trans_http_endpoint_t)mgr;
        TAILQ_INSERT_TAIL(&mgr->m_free_tasks, task, m_next_for_mgr);
        task = NULL;
    }
    
    if (http_ep) {
        if (!net_trans_http_endpoint_is_active(http_ep)) {
            net_trans_http_endpoint_free(http_ep);
        }
        http_ep = NULL;
    }
    
    if (host) {
        if (TAILQ_EMPTY(&host->m_endpoints)) {
            net_trans_host_free(host);
        }
        host = NULL;
    }

    if (address) {
        net_address_free(address);
        address = NULL;
    }
    
    return NULL;
}

void net_trans_task_free(net_trans_task_t task) {
    net_trans_http_endpoint_t http_ep = task->m_ep;
    net_trans_manage_t mgr = http_ep->m_host->m_mgr;

    if (task->m_in_callback) {
        task->m_is_free = 1;
        return;
    }

    if (task->m_ctx_free) {
        task->m_ctx_free(task->m_ctx);
        task->m_ctx = NULL;
        task->m_ctx_free = NULL;
    }

    if (task->m_ep) {
        TAILQ_REMOVE(&task->m_ep->m_tasks, task, m_next_for_ep);
        task->m_ep = NULL;
    }
    
    cpe_hash_table_remove_by_ins(&mgr->m_tasks, task);

    task->m_ep = (net_trans_http_endpoint_t)mgr;
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
    net_trans_manage_t mgr = (net_trans_manage_t)task->m_ep;

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
    return task->m_ep->m_host->m_mgr;
}

net_trans_task_state_t net_trans_task_state(net_trans_task_t task) {
    if (task->m_http_req) {
        switch(net_http_req_state(task->m_http_req)) {
        case net_http_req_state_completed:
            return net_trans_task_working;
        default:
            return net_trans_task_init;
        }
    }
    else {
        return net_trans_task_done;
    }
}

net_trans_task_result_t net_trans_task_result(net_trans_task_t task) {
    return task->m_result;
}

int16_t net_trans_task_res_code(net_trans_task_t task) {
    return task->m_res_code;
}

const char * net_trans_task_res_mine(net_trans_task_t task) {
    return task->m_res_mine;
}

const char * net_trans_task_res_charset(net_trans_task_t task) {
    return task->m_res_charset;
}

void net_trans_task_set_debug(net_trans_task_t task, uint8_t is_debug) {
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

int net_trans_task_set_timeout(net_trans_task_t task, uint64_t timeout_ms);

int net_trans_task_append_header(net_trans_task_t task, const char * header_one);

int net_trans_task_start(net_trans_task_t task) {
    net_trans_manage_t mgr = task->m_ep->m_host->m_mgr;

    if (net_http_req_set_reader(
            task->m_http_req,
            task,
            NULL,
            NULL,
            task->m_write_op ? net_trans_task_on_body : NULL,
            net_trans_task_on_complete) != 0)
    {
        CPE_ERROR(mgr->m_em, "trans: task: set reader fail!");
        return -1;
    }
    
    if (!net_trans_http_endpoint_is_active(task->m_ep)) {
        net_http_endpoint_enable(net_http_endpoint_from_data(task->m_ep));
    }
    
    if (net_http_req_write_commit(task->m_http_req) != 0) {
        CPE_ERROR(mgr->m_em, "trans: task: start fail!");
        return -1;
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
        net_address_set_port(address, *is_https ? 433 : 80);
    }
    
    return address;
}

static net_http_res_op_result_t net_trans_task_on_body(void * ctx, net_http_req_t req, void * data, size_t data_size) {
    net_trans_task_t task = ctx;

    if (task->m_write_op) {
        task->m_in_callback = 1;
        task->m_write_op(task, task->m_ctx, data, data_size);
        task->m_in_callback = 0;

        if (task->m_is_free) {
            net_trans_task_free(task);
        }
    }
    
    return net_http_res_op_success;
}

static net_http_res_op_result_t net_trans_task_on_complete(void * ctx, net_http_req_t req, net_http_res_result_t result) {
    net_trans_task_t task = ctx;
    net_trans_manage_t mgr = task->m_ep->m_host->m_mgr;

    switch(result) {
    case net_http_res_complete:
        task->m_result = net_trans_result_complete;
        task->m_res_code = net_http_req_res_code(req);
        break;
    case net_http_res_timeout:
        task->m_result = net_trans_result_timeout;
        break;
    case net_http_res_canceled:
        task->m_result = net_trans_result_cancel;
        break;
    case net_http_res_disconnected:
        task->m_result = net_trans_result_cancel;
        break;
    }

    assert(task->m_ep);
    TAILQ_REMOVE(&task->m_ep->m_tasks, task, m_next_for_ep);
    task->m_ep = NULL;
    
    task->m_http_req = NULL;
    if (task->m_commit_op) {
        uint32_t buf_sz = 0;
        void * buf = NULL;
        
        if (task->m_write_op == NULL) {
            net_endpoint_t base_endpoint = net_http_endpoint_net_ep(net_http_req_ep(req));
            buf_sz = net_endpoint_buf_size(base_endpoint, net_ep_buf_http_body);

            if (net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_http_body, buf_sz, &buf) != 0) {
                CPE_ERROR(mgr->m_em, "trans: task %d: complete: get http data fail, size=%d!", task->m_id, buf_sz);
                task->m_result = net_trans_result_cancel;
                buf = NULL;
                buf_sz = 0;
            }
        }

        task->m_in_callback = 1;
        task->m_commit_op(task, task->m_ctx, buf, buf_sz);
        task->m_in_callback = 0;

        if (task->m_is_free) {
            net_trans_task_free(task);
        }
    }
    
    return net_http_res_op_success;
}
