#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/url.h"
#include "cpe/utils/string_utils.h"
#include "net_trans_manage.h"
#include "net_trans_task.h"
#include "url_runner.h"

static void url_runner_libcurl_task_commit(net_trans_task_t task, void * ctx, void * data, size_t data_size);
static void url_runner_libcurl_task_progress(net_trans_task_t task, void * ctx, double dltotal, double dlnow);
static void url_runner_libcurl_task_write(net_trans_task_t task, void * ctx, void * data, size_t data_size);
static void net_runner_libcurl_head(net_trans_task_t task, void * ctx, const char * name, const char * value);

int url_runner_libcurl_init(url_runner_t runner) {
    runner->m_libcurl.m_trans_mgr =
        net_trans_manage_create(
            runner->m_alloc, runner->m_em, runner->m_net_schedule, runner->m_net_driver, "tool");
    if (runner->m_internal.m_http_protocol == NULL) {
        CPE_ERROR(runner->m_em, "tool: create http protocol fail");
        return -1;
    }
    
    runner->m_libcurl.m_trans_task = NULL;
    return 0;
}

void url_runner_libcurl_fini(url_runner_t runner) {
    if (runner->m_libcurl.m_trans_task) {
        net_trans_task_clear_callback(runner->m_libcurl.m_trans_task);
        net_trans_task_free(runner->m_libcurl.m_trans_task);
        runner->m_libcurl.m_trans_task = NULL;
    }

    if (runner->m_libcurl.m_trans_mgr) {
        net_trans_manage_free(runner->m_libcurl.m_trans_mgr);
        runner->m_libcurl.m_trans_mgr = NULL;
    }
}

int url_runner_libcurl_start(
    url_runner_t runner, const char * str_method, const char * url,
    const char * header[], uint16_t header_count, const char * body)
{
    net_trans_method_t method = net_trans_method_get;

    if (strcasecmp(str_method, "get") == 0) {
        method = net_trans_method_get;
    }
    else if (strcasecmp(str_method, "post") == 0) {
        method = net_trans_method_post;
    }
    else if (strcasecmp(str_method, "put") == 0) {
        method = net_trans_method_put;
    }
    else if (strcasecmp(str_method, "delete") == 0) {
        method = net_trans_method_delete;
    }
    else if (strcasecmp(str_method, "patch") == 0) {
        method = net_trans_method_patch;
    }
    else if (strcasecmp(str_method, "head") == 0) {
        method =net_trans_method_head;
    }
    
    runner->m_libcurl.m_trans_task =
        net_trans_task_create(
            runner->m_libcurl.m_trans_mgr,
            method,
            url,
            1);
	if (runner->m_libcurl.m_trans_task == NULL) {
		CPE_ERROR(runner->m_em, "tool: libcurl: task create fail");
        return -1;
	}

    net_trans_task_set_callback(
        runner->m_libcurl.m_trans_task,
        url_runner_libcurl_task_commit,
        url_runner_libcurl_task_progress,
        url_runner_libcurl_task_write,
        net_runner_libcurl_head,
        runner, NULL);
    
    uint32_t i;
    for(i = 0; i < header_count; ++i) {
        const char * head_line = header[i];
        const char * sep = strchr(head_line, ':');

        if (sep == NULL) {
            CPE_ERROR(runner->m_em, "url: head format error: %s", head_line);
            continue;
        }

        sep = cpe_str_trim_tail((char*)sep, head_line);
        char * head_name = cpe_str_mem_dup_range(NULL, head_line, sep);
        net_trans_task_append_header(
            runner->m_libcurl.m_trans_task,
            head_name, cpe_str_trim_head((char*)(sep + 1)));
    }

    if (body) {
        if (net_trans_task_set_body(runner->m_libcurl.m_trans_task, body, strlen(body)) != 0) return -1;
    }
    
    if (net_trans_task_start(runner->m_libcurl.m_trans_task) != 0) return -1;

    return 0;
}

static void url_runner_libcurl_task_commit(net_trans_task_t task, void * ctx, void * data, size_t data_size) {
    url_runner_t runner = ctx;
    assert(runner->m_mode == url_runner_mode_libcurl);

    assert(runner->m_libcurl.m_trans_task == task);
    runner->m_libcurl.m_trans_task = NULL;

    net_trans_task_free(task);
    url_runner_loop_break(runner);
}

static void url_runner_libcurl_task_progress(net_trans_task_t task, void * ctx, double dltotal, double dlnow) {
}

static void url_runner_libcurl_task_write(net_trans_task_t task, void * ctx, void * data, size_t data_size) {
    url_runner_t runner = ctx;
    fprintf(runner->m_output, "%.*s\n", (int)data_size, (const char *)data);
}

static void net_runner_libcurl_head(net_trans_task_t task, void * ctx, const char * name, const char * value) {
    url_runner_t runner = ctx;
    fprintf(runner->m_output, "%s: %s\n", name, value);
}

