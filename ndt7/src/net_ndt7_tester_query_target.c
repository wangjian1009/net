#include <assert.h>
#include "net_endpoint.h"
#include "net_address.h"
#include "net_http_endpoint.h"
#include "net_http_req.h"
#include "net_ndt7_tester_i.h"

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
    net_endpoint_set_protocol_debug(base_endpoint, 2);
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

    //"https://locate.measurementlab.net/v2/nearest/ndt/ndt7?client_name=ndt7-android&client_version=${BuildConfig.NDT7_ANDROID_VERSION_NAME}" */
    tester->m_state_data.m_query_target.m_req =
        net_http_req_create(
            tester->m_state_data.m_query_target.m_endpoint,
            net_http_req_method_get,
            "/v2/nearest/ndt/ndt7?client_name=ndt7-android&client_version=${BuildConfig.NDT7_ANDROID_VERSION_NAME}");
	if (tester->m_state_data.m_query_target.m_req == NULL) {
		CPE_ERROR(manager->m_em, "ndt7: %d: query target: create http req fail", tester->m_id);
        return -1;
	}

    if (net_http_req_set_reader(
            tester->m_state_data.m_query_target.m_req, tester, NULL, NULL, NULL,
            net_ndt7_tester_query_tearget_on_req_complete) != 0
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
}

static void net_ndt7_tester_query_tearget_on_req_complete(
    void * ctx, net_http_req_t http_req, net_http_res_result_t result, void * body, uint32_t body_size)
{
}

