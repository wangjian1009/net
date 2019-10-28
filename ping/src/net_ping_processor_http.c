#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "net_trans_task.h"
#include "net_ping_processor_i.h"

static void net_ping_processor_http_task_commit(net_trans_task_t task, void * ctx, void * data, size_t data_size);

int net_ping_processor_start_http(net_ping_processor_t processor) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = task->m_mgr;

    assert(processor->m_http.m_task == NULL);

    processor->m_http.m_task = net_trans_task_create(mgr->m_trans_mgr, net_trans_method_get, task->m_http.m_url, task->m_debug);
    if (processor->m_http.m_task == NULL) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: create trans task fail!",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
        return -1;
    }

    net_trans_task_set_callback(
        processor->m_http.m_task,
        net_ping_processor_http_task_commit, NULL, NULL, NULL,
        processor, NULL);

    if (net_trans_task_start(processor->m_http.m_task) != 0) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: start trans task fail!",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
        net_trans_task_free(processor->m_http.m_task);
        processor->m_http.m_task = NULL;
        return -1;
    }

    return 0;
}

static void net_ping_processor_http_task_commit(net_trans_task_t trans_task, void * ctx, void * data, size_t data_size) {
    net_ping_processor_t processor = ctx;

    assert(processor->m_http.m_task == trans_task);
    
    switch(net_trans_task_result(trans_task)) {
    case net_trans_result_unknown:
        net_point_processor_set_result_one(processor, net_ping_error_internal, "unknown-trans-task", 1, 0, 0, 0);
        break;
    case net_trans_result_complete:
        net_point_processor_set_result_one(processor, net_ping_error_none, NULL, 1, 0, 0, 0);
        break;
    case net_trans_result_error:
        switch(net_trans_task_error(trans_task)) {
        case net_trans_task_error_none:
            net_point_processor_set_result_one(processor, net_ping_error_none, NULL, 1, 0, 0, 0);
            break;
        case net_trans_task_error_timeout:
        case net_trans_task_error_remote_reset:
        case net_trans_task_error_dns_resolve_fail:
        case net_trans_task_error_net_unreachable:
        case net_trans_task_error_net_down:
        case net_trans_task_error_host_unreachable:
        case net_trans_task_error_connect:
        case net_trans_task_error_internal:
            net_point_processor_set_result_one(
                processor, net_ping_error_internal,
                net_trans_task_error_str(net_trans_task_error(trans_task)), 1, 0, 0, 0);
            break;
        }
        break;
    case net_trans_result_cancel:
        net_point_processor_set_result_one(processor, net_ping_error_internal, "cancel", 1, 0, 0, 0);
        break;
    }
    
    processor->m_http.m_task = NULL;
    net_trans_task_free(trans_task);
}
