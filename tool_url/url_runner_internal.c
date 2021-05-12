#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/url.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_endpoint.h" 
#include "net_driver.h"
#include "net_ssl_stream_driver.h"
#include "net_http_protocol.h"
#include "net_http_endpoint.h"
#include "net_http_req.h"
#include "url_runner.h"

static void url_runner_internal_on_endpoint_fini(void * ctx, net_endpoint_t endpoint);
static int url_runner_internal_on_res_begin(void * ctx, net_http_req_t req, uint16_t code, const char * msg);
static int url_runner_internal_on_res_head(void * ctx, net_http_req_t req, const char * name, const char * value);
static int url_runner_internal_on_res_body(void * ctx, net_http_req_t req, void * data, uint32_t data_size);

static void url_runner_internal_on_req_complete(
    void * ctx, net_http_req_t req, net_http_res_result_t result, void * body, uint32_t body_size);

int url_runner_internal_init(url_runner_t runner) {
    runner->m_internal.m_http_protocol = net_http_protocol_create(runner->m_net_schedule, "tool");
    if (runner->m_internal.m_http_protocol == NULL) {
        CPE_ERROR(runner->m_em, "tool: create http protocol fail");
        return -1;
    }
    
    net_ssl_stream_driver_t stream_driver =
        net_ssl_stream_driver_create(
            runner->m_net_schedule, "tool", runner->m_net_driver, runner->m_alloc, runner->m_em);
    if (stream_driver == NULL) {
        CPE_ERROR(runner->m_em, "tool: create ssl driver fail");
        net_http_protocol_free(runner->m_internal.m_http_protocol);
        runner->m_internal.m_http_protocol = NULL;
        return -1;
    }

    runner->m_internal.m_ssl_driver = net_driver_from_data(stream_driver);
    runner->m_internal.m_http_endpoint = NULL;
    runner->m_internal.m_http_req = NULL;
    return 0;
}

void url_runner_internal_fini(url_runner_t runner) {
    if (runner->m_internal.m_http_req) {
        net_http_req_clear_reader(runner->m_internal.m_http_req);
        net_http_req_free(runner->m_internal.m_http_req);
        runner->m_internal.m_http_req = NULL;
    }
    
    if (runner->m_internal.m_http_endpoint) {
        net_endpoint_set_data_watcher(
            net_http_endpoint_base_endpoint(runner->m_internal.m_http_endpoint),
            NULL, NULL, NULL);
        net_http_endpoint_free(runner->m_internal.m_http_endpoint);
        runner->m_internal.m_http_endpoint = NULL;
    }

    if (runner->m_internal.m_ssl_driver) {
        net_driver_free(runner->m_internal.m_ssl_driver);
        runner->m_internal.m_ssl_driver = NULL;
    }
    
    if (runner->m_internal.m_http_protocol) {
        net_http_protocol_free(runner->m_internal.m_http_protocol);
        runner->m_internal.m_http_protocol = NULL;
    }
}

int url_runner_internal_create_endpoint(url_runner_t runner, const char * str_url) {
    cpe_url_t url = cpe_url_parse(runner->m_alloc, runner->m_em, str_url);
    if (url == NULL) {
        CPE_ERROR(runner->m_em, "tool: url %s format error", str_url);
        return -1;
    }

    net_driver_t driver = NULL;
    
    if (strcasecmp(cpe_url_protocol(url), "http") == 0) {
        driver = runner->m_net_driver;
    }
    else if (strcasecmp(cpe_url_protocol(url), "https") == 0) {
        driver = runner->m_internal.m_ssl_driver;
    }
    else {
        CPE_ERROR(runner->m_em, "url: not support protocol %s", cpe_url_protocol(url));
        cpe_url_free(url);
        return -1;
    }
    
    runner->m_internal.m_http_endpoint = net_http_endpoint_create(driver, runner->m_internal.m_http_protocol);
    if (runner->m_internal.m_http_endpoint == NULL) {
        CPE_ERROR(runner->m_em, "tool: create endpoint fail");
        cpe_url_free(url);
        return -1;
    }
    net_endpoint_set_auto_free(net_http_endpoint_base_endpoint(runner->m_internal.m_http_endpoint), 1);

    net_endpoint_t base_endpoint = net_http_endpoint_base_endpoint(runner->m_internal.m_http_endpoint);
    net_endpoint_set_protocol_debug(base_endpoint, 2);
    net_endpoint_set_driver_debug(base_endpoint, 2);

    net_address_t address = net_address_create_from_url(runner->m_net_schedule, url);
    if (address == NULL) {
        CPE_ERROR(runner->m_em, "tool: create address from url %s fail", str_url);
        cpe_url_free(url);
        return -1;
    }
    cpe_url_free(url);
    
    if (net_endpoint_set_remote_address(base_endpoint, address)) {
        CPE_ERROR(runner->m_em, "tool: set remote address fail");
        net_address_free(address);
        return -1;
    }
    net_address_free(address);

    net_endpoint_set_data_watcher(
        base_endpoint,
        runner,
        NULL,
        url_runner_internal_on_endpoint_fini);
    
    return 0;
}

int url_runner_internal_start_req(
    url_runner_t runner,
    const char * str_method,
    const char * url,
    const char * header[], uint16_t header_count, 
    const char * body)
{
    net_http_req_method_t method = net_http_req_method_get;

    if (strcasecmp(str_method, "get") == 0) {
        method = net_http_req_method_get;
    }
    else if (strcasecmp(str_method, "post") == 0) {
        method = net_http_req_method_post;
    }
    else if (strcasecmp(str_method, "put") == 0) {
        method = net_http_req_method_put;
    }
    else if (strcasecmp(str_method, "delete") == 0) {
        method = net_http_req_method_delete;
    }
    else if (strcasecmp(str_method, "patch") == 0) {
        method = net_http_req_method_patch;
    }
    else if (strcasecmp(str_method, "head") == 0) {
        method =net_http_req_method_head;
    }
    
    runner->m_internal.m_http_req =
        net_http_req_create(
            runner->m_internal.m_http_endpoint,
            method,
            url);
	if (runner->m_internal.m_http_req == NULL) {
		CPE_ERROR(runner->m_em, "tool: internal: http req create fail");
        return -1;
	}

    if (net_http_req_set_reader(
            runner->m_internal.m_http_req,
            runner,
            url_runner_internal_on_res_begin,
            url_runner_internal_on_res_head,
            url_runner_internal_on_res_body,
            url_runner_internal_on_req_complete) != 0) return -1;

    if (net_http_req_write_head_host(runner->m_internal.m_http_req) != 0) return -1;

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
        net_http_req_write_head_pair(
            runner->m_internal.m_http_req,
            head_name, cpe_str_trim_head((char*)(sep + 1)));
    }

    if (body) {
        if (net_http_req_write_body_full(runner->m_internal.m_http_req, body, strlen(body)) != 0) return -1;
    }
    
    if (net_http_req_write_commit(runner->m_internal.m_http_req) != 0) return -1;
    
    return 0;
}

int url_runner_internal_start(
    url_runner_t runner, const char * method, const char * url,
    const char * header[], uint16_t header_count, const char * body)
{
    if (url_runner_internal_create_endpoint(runner, url) != 0) return -1;
    if (url_runner_internal_start_req(runner, method, url, header, header_count, body) != 0) return -1;

    if (net_endpoint_connect(
            net_http_endpoint_base_endpoint(runner->m_internal.m_http_endpoint)) != 0) return -1;

    return 0;
}

static void url_runner_internal_on_endpoint_fini(void * ctx, net_endpoint_t endpoint) {
    url_runner_t runner = ctx;
    runner->m_internal.m_http_endpoint = NULL;
}

static int url_runner_internal_on_res_begin(void * ctx, net_http_req_t req, uint16_t code, const char * msg) {
    url_runner_t runner = ctx;
    fprintf(runner->m_output, "%d %s\n", code, msg);
    return 0;
}

static int url_runner_internal_on_res_head(void * ctx, net_http_req_t req, const char * name, const char * value) {
    url_runner_t runner = ctx;
    fprintf(runner->m_output, "%s: %s\n", name, value);
    return 0;
}
    
static int url_runner_internal_on_res_body(void * ctx, net_http_req_t req, void * data, uint32_t data_size) {
    url_runner_t runner = ctx;
    fprintf(runner->m_output, "%.*s\n", (int)data_size, (const char *)data);
    return 0;
}

static void url_runner_internal_on_req_complete(
    void * ctx, net_http_req_t req, net_http_res_result_t result, void * body, uint32_t body_size)
{
    url_runner_t runner = ctx;
    assert(runner->m_mode == url_runner_mode_internal);

    assert(runner->m_internal.m_http_req == req);
    runner->m_internal.m_http_req = NULL;

    net_http_req_free(req);
    url_runner_loop_break(runner);
}
