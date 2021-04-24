#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/url.h"
#include "net_address.h"
#include "net_endpoint.h" 
#include "net_ssl_stream_driver.h"
#include "net_http_protocol.h"
#include "net_http_endpoint.h"
#include "net_http_req.h"
#include "url_runner.h"

static void url_runner_internal_on_endpoint_fini(void * ctx, net_endpoint_t endpoint);
static void url_runner_internal_on_req_complete(
    void * ctx, net_http_req_t req, net_http_res_result_t result, void * body, uint32_t body_size);

int url_runner_internal_init(url_runner_t runner) {
    runner->m_internal.m_http_protocol = net_http_protocol_create(runner->m_net_schedule, "tool");
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
    
    if (runner->m_internal.m_http_protocol) {
        net_http_protocol_free(runner->m_internal.m_http_protocol);
        runner->m_internal.m_http_protocol = NULL;
    }
}

int url_runner_internal_create_endpoint(url_runner_t runner, const char * str_url) {
    runner->m_internal.m_http_endpoint =
        net_http_endpoint_create(
            runner->m_net_driver, runner->m_internal.m_http_protocol);
    if (runner->m_internal.m_http_endpoint == NULL) {
        CPE_ERROR(runner->m_em, "tool: create endpoint fail");
        return -1;
    }
    net_http_endpoint_set_auto_free(runner->m_internal.m_http_endpoint, 1);

    net_endpoint_t base_endpoint = net_http_endpoint_base_endpoint(runner->m_internal.m_http_endpoint);
    net_endpoint_set_protocol_debug(base_endpoint, 2);
    net_endpoint_set_driver_debug(base_endpoint, 2);

    cpe_url_t url = cpe_url_parse(runner->m_alloc, runner->m_em, str_url);
    if (url == NULL) {
        CPE_ERROR(runner->m_em, "tool: url %s format error", str_url);
        return -1;
    }
    
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
    const char * method,
    const char * url,
    const char * body)
{
    runner->m_internal.m_http_req =
        net_http_req_create(
            runner->m_internal.m_http_endpoint,
            net_http_req_method_get,
            url);
	if (runner->m_internal.m_http_req == NULL) {
		CPE_ERROR(runner->m_em, "tool: internal: http req create fail");
        return -1;
	}

    if (net_http_req_set_reader(
            runner->m_internal.m_http_req, runner, NULL, NULL, NULL,
            url_runner_internal_on_req_complete) != 0
        || net_http_req_write_head_pair(runner->m_internal.m_http_req, "Content-Type", "application/json") != 0
        || net_http_req_write_head_host(runner->m_internal.m_http_req)
        || (body
            && net_http_req_write_body_full(runner->m_internal.m_http_req, body, strlen(body)) != 0)
        || net_http_req_write_commit(runner->m_internal.m_http_req) != 0)
    {
        CPE_ERROR(runner->m_em, "tool: internal: http start req fail!");
        return -1;
    }
    
    return 0;
}

int url_runner_internal_start(
    url_runner_t runner, const char * method, const char * url, const char * body)
{
    if (url_runner_internal_create_endpoint(runner, url) != 0) return -1;
    if (url_runner_internal_start_req(runner, method, url, body) != 0) return -1;

    if (net_endpoint_connect(
            net_http_endpoint_base_endpoint(runner->m_internal.m_http_endpoint)) != 0) return -1;

    return 0;
}

static void url_runner_internal_on_endpoint_fini(void * ctx, net_endpoint_t endpoint) {
    url_runner_t runner = ctx;
    runner->m_internal.m_http_endpoint = NULL;
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
