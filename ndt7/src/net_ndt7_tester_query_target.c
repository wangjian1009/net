#include <assert.h>
#include "yajl/yajl_tree.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_http_endpoint.h"
#include "net_http_req.h"
#include "net_ndt7_tester_i.h"
#include "net_ndt7_tester_target_i.h"

static void net_ndt7_tester_query_target_on_endpoint_fini(void * ctx, net_endpoint_t endpoint);
static void net_ndt7_tester_query_tearget_on_req_complete(
    void * ctx, net_http_req_t http_req, net_http_res_result_t result, void * body, uint32_t body_size);

int net_ndt7_tester_query_target_start(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    assert(tester->m_state == net_ndt7_tester_state_query_target);
    assert(tester->m_state_data.m_query_target.m_endpoint == NULL);
    assert(tester->m_state_data.m_query_target.m_req == NULL);

    tester->m_state_data.m_query_target.m_endpoint
        = net_http_endpoint_create(manager->m_ssl_driver, manager->m_http_protocol);
    if (tester->m_state_data.m_query_target.m_endpoint == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: query target: endpoint fail", tester->m_id);
        return -1;
    }
    net_http_endpoint_set_auto_free(tester->m_state_data.m_query_target.m_endpoint, 1);

    net_endpoint_t base_endpoint = net_http_endpoint_base_endpoint(tester->m_state_data.m_query_target.m_endpoint);
    /* net_endpoint_set_protocol_debug(base_endpoint, 2); */
    /* net_endpoint_set_driver_debug(base_endpoint, 2); */

    net_endpoint_set_data_watcher(
        base_endpoint,
        tester,
        NULL,
        net_ndt7_tester_query_target_on_endpoint_fini);

    net_address_t address =
        net_address_create_auto(manager->m_schedule, "locate.measurementlab.net:443");
    if (address == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: query target: create address fail", tester->m_id);
        return -1;
    }

    if (net_endpoint_set_remote_address(base_endpoint, address) != 0) {
        CPE_ERROR(manager->m_em, "ndt7: %d: query target: set remote address fail", tester->m_id);
        net_address_free(address);
        return -1;
    }
    net_address_free(address);

    if (net_endpoint_connect(base_endpoint) != 0) {
        CPE_ERROR(manager->m_em, "ndt7: %d: query target: start connect fail", tester->m_id);
        return -1;
    }
    
    //"https://locate.measurementlab.net/v2/nearest/ndt/ndt7?client_name=ndt7-android&client_version=${BuildConfig.NDT7_ANDROID_VERSION_NAME}" */
    tester->m_state_data.m_query_target.m_req =
        net_http_req_create(
            tester->m_state_data.m_query_target.m_endpoint,
            net_http_req_method_get,
            "/v2/nearest/ndt/ndt7");
	if (tester->m_state_data.m_query_target.m_req == NULL) {
		CPE_ERROR(manager->m_em, "ndt7: %d: query target: create http req fail", tester->m_id);
        return -1;
	}

    if (net_http_req_set_reader(
            tester->m_state_data.m_query_target.m_req,
            tester, NULL, NULL, NULL,
            net_ndt7_tester_query_tearget_on_req_complete) != 0
        || net_http_req_write_head_host(tester->m_state_data.m_query_target.m_req) != 0
        || net_http_req_write_head_pair(tester->m_state_data.m_query_target.m_req, "Content-Type", "application/json") != 0
        || net_http_req_write_head_host(tester->m_state_data.m_query_target.m_req)
        || net_http_req_write_commit(tester->m_state_data.m_query_target.m_req) != 0)
    {
		CPE_ERROR(manager->m_em, "ndt7: %d: query target: start http req fail", tester->m_id);
        return -1;
    }
    
    return 0;
}

static void net_ndt7_tester_query_target_on_endpoint_fini(void * ctx, net_endpoint_t endpoint) {
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    assert(tester->m_state == net_ndt7_tester_state_query_target);
    assert(tester->m_state_data.m_query_target.m_endpoint);
    assert(net_http_endpoint_base_endpoint(tester->m_state_data.m_query_target.m_endpoint) == endpoint);

    tester->m_state_data.m_query_target.m_endpoint = NULL;
}

static int net_ndt7_tester_query_tearget_build_target(net_ndt7_tester_t tester, yajl_val config) {
    net_ndt7_manage_t manager = tester->m_manager;
    const char * str_val;
    
    net_ndt7_tester_target_t target = net_ndt7_tester_target_create(tester);
    if (target == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: query target: response: create target error", tester->m_id);
        return -1;
    }

    if ((str_val = yajl_get_string(yajl_tree_get_2(config, "machine", yajl_t_string)))) {
        net_ndt7_tester_target_set_machine(target, str_val);
    }

    if ((str_val = yajl_get_string(yajl_tree_get_2(config, "location/city", yajl_t_string)))) {
        net_ndt7_tester_target_set_city(target, str_val);
    }
    
    if ((str_val = yajl_get_string(yajl_tree_get_2(config, "location/country", yajl_t_string)))) {
        net_ndt7_tester_target_set_country(target, str_val);
    }

    const char * path1[] = { "urls", "ws:///ndt/v7/download", NULL };
    if ((str_val = yajl_get_string(yajl_tree_get(config, path1, yajl_t_string)))) {
        if (net_ndt7_tester_target_set_url(target, net_ndt7_target_url_ws_download, str_val) != 0) return -1;
    }

    const char * path2[] = { "urls", "ws:///ndt/v7/upload", NULL };
    if ((str_val = yajl_get_string(yajl_tree_get(config, path2, yajl_t_string)))) {
        if (net_ndt7_tester_target_set_url(target, net_ndt7_target_url_ws_uploded, str_val) != 0) return -1;
    }

    const char * path3[] = { "urls", "wss:///ndt/v7/download", NULL };
    if ((str_val = yajl_get_string(yajl_tree_get(config, path3, yajl_t_string)))) {
        if (net_ndt7_tester_target_set_url(target, net_ndt7_target_url_wss_download, str_val) != 0) return -1;
    }

    const char * path4[] = { "urls", "wss:///ndt/v7/upload", NULL };
    if ((str_val = yajl_get_string(yajl_tree_get(config, path4, yajl_t_string)))) {
        if (net_ndt7_tester_target_set_url(target, net_ndt7_target_url_wss_uploded, str_val) != 0) return -1;
    }
    
    return 0;
}

static void net_ndt7_tester_query_tearget_on_req_complete(
    void * ctx, net_http_req_t http_req, net_http_res_result_t result, void * body, uint32_t body_size)
{
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    switch(result) {
    case net_http_res_complete:
        break;
    case net_http_res_timeout:
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_timeout, "QueryTargetTimeout");
        net_ndt7_tester_check_start_next_step(tester);
        return;
    case net_http_res_canceled:
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_cancel, "QueryTargetCancel");
        net_ndt7_tester_check_start_next_step(tester);
        return;
    case net_http_res_conn_error:
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_network, "QueryTargetNetwork");
        net_ndt7_tester_check_start_next_step(tester);
        return;
    case net_http_res_conn_disconnected:
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_network, "QueryTargetDisconnected");
        net_ndt7_tester_check_start_next_step(tester);
        return;
    }

    if (net_http_req_res_code(http_req) / 100 != 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "%d(%s)", net_http_req_res_code(http_req), net_http_req_res_message(http_req));
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_network, error_msg);
        net_ndt7_tester_check_start_next_step(tester);
        return;
    }

    if (body == NULL) {
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_network, "NoBodyData");
        net_ndt7_tester_check_start_next_step(tester);
        return;
    }
    
    char error_buf[256];
    yajl_val content = yajl_tree_parse_len(body, body_size, error_buf, sizeof(error_buf));
    if (content == NULL) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: query target: parse response fail, error=%s\n%.*s!",
            tester->m_id, error_buf, (int)body_size, (const char *)body);
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_network, "BodyParseError");
        net_ndt7_tester_check_start_next_step(tester);
        return;
    }

    yajl_val results_val = yajl_tree_get_2(content, "results", yajl_t_array);
    if (results_val == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: query target: response no results", tester->m_id);
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_network, "BodyParseError");
        yajl_tree_free(content);
        net_ndt7_tester_check_start_next_step(tester);
        return;
    }

    if (results_val->u.array.len == 0) {
        CPE_ERROR(manager->m_em, "ndt7: %d: query target: response results empty", tester->m_id);
        net_ndt7_tester_set_error(tester, net_ndt7_tester_error_network, "ResultsEmpty");
        yajl_tree_free(content);
        net_ndt7_tester_check_start_next_step(tester);
        return;
    }
    
    uint32_t i;
    for(i = 0; i < results_val->u.array.len; ++i) {
        if (net_ndt7_tester_query_tearget_build_target(tester, results_val->u.array.values[i]) != 0) {
            CPE_ERROR(manager->m_em, "ndt7: %d: query target: response results empty", tester->m_id);
            net_ndt7_tester_set_error(tester, net_ndt7_tester_error_network, "ResultFormatError");
            yajl_tree_free(content);
            net_ndt7_tester_check_start_next_step(tester);
            return;
        }
    }

    yajl_tree_free(content);

    net_ndt7_tester_check_start_next_step(tester);
}
