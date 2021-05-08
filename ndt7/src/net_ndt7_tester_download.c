#include <assert.h>
#include "yajl/yajl_tree.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/url.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_endpoint_monitor.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_ws_endpoint.h"
#include "net_ndt7_tester_i.h"
#include "net_ndt7_model_i.h"
#include "net_ndt7_json_i.h"

static void net_ndt7_tester_download_on_msg_text(void * ctx, net_ws_endpoint_t endpoin, const char * msg);
static void net_ndt7_tester_download_on_msg_bin(
    void * ctx, net_ws_endpoint_t endpoin, const void * msg, uint32_t msg_len);
static void net_ndt7_tester_download_on_close(
    void * ctx, net_ws_endpoint_t endpoin, uint16_t status_code, const void * msg, uint32_t msg_len);

static void net_ndt7_tester_download_on_endpoint_fini(void * ctx, net_endpoint_t endpoint);
static void net_ndt7_tester_download_on_endpoint_evt(
    void * ctx, net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt);

static void net_ndt7_tester_download_try_notify_update(net_ndt7_tester_t tester);

int net_ndt7_tester_download_start(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    assert(tester->m_state == net_ndt7_tester_state_download);

    if (tester->m_download_url == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: download: start: no download url", tester->m_id);
        net_ndt7_tester_set_error_internal(tester, "NoDownloadUrl");
        return -1;
    }

    net_driver_t driver = NULL;
    if (strcasecmp(cpe_url_protocol(tester->m_download_url), "ws") == 0) {
        driver = manager->m_base_driver;
    }
    else if (strcasecmp(cpe_url_protocol(tester->m_download_url), "wss") == 0) {
        driver = manager->m_ssl_driver;
    }
    else {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: download: start: not support protocol %s",
            tester->m_id, cpe_url_protocol(tester->m_download_url));
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "NotSupportProtocol(%s)", cpe_url_protocol(tester->m_download_url));
        net_ndt7_tester_set_error_internal(tester, error_msg);
        return -1;
    }
    
    net_endpoint_t base_endpoint =
        net_endpoint_create(driver, net_protocol_from_data(manager->m_ws_protocol), NULL);
    if (base_endpoint == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: download: start: create protocol fail", tester->m_id);
        net_ndt7_tester_set_error_internal(tester, "CreateEndpointFail");
        return -1;
    }
    tester->m_download.m_endpoint = net_ws_endpoint_cast(base_endpoint);
    assert(tester->m_download.m_endpoint);
    //net_endpoint_set_protocol_debug(base_endpoint, 2);
    net_endpoint_set_auto_free(base_endpoint, 1);

    if (net_endpoint_monitor_create(base_endpoint, tester, NULL, net_ndt7_tester_download_on_endpoint_evt) == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: download: art: create state monitor fail!", tester->m_id);
        net_ndt7_tester_set_error_internal(tester, "CreateEndpointMonitorFail");
        goto START_FAIL;
    }
    
    net_endpoint_set_data_watcher(
        base_endpoint,
        tester,
        NULL,
        net_ndt7_tester_download_on_endpoint_fini);

    net_ws_endpoint_header_add(
        tester->m_download.m_endpoint,
        "Sec-WebSocket-Protocol", "net.measurementlab.ndt.v7");
    
    net_ws_endpoint_set_callback(
        tester->m_download.m_endpoint,
        tester,
        net_ndt7_tester_download_on_msg_text,
        net_ndt7_tester_download_on_msg_bin,
        net_ndt7_tester_download_on_close,
        NULL);

    if (net_ws_endpoint_connect(tester->m_download.m_endpoint, tester->m_download_url) != 0) {
        CPE_ERROR(manager->m_em, "ndt7: %d: download: start: connect start fail", tester->m_id);
        goto START_FAIL;
    }

    tester->m_download.m_start_time_ms = net_schedule_cur_time_ms(manager->m_schedule);
    tester->m_download.m_pre_notify_ms = tester->m_download.m_start_time_ms;

    return 0;

START_FAIL:
    if (tester->m_download.m_endpoint) {
        net_endpoint_set_data_watcher(base_endpoint, NULL, NULL, NULL);
        net_endpoint_monitor_free_by_ctx(base_endpoint, tester);
        net_ws_endpoint_free(tester->m_download.m_endpoint);
        tester->m_download.m_endpoint = NULL;
    }
    
    return -1;
}

static void net_ndt7_tester_download_on_msg_text(void * ctx, net_ws_endpoint_t endpoin, const char * msg) {
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    tester->m_download.m_num_bytes += strlen(msg);
    net_ndt7_tester_download_try_notify_update(tester);

    char error_buf[256];
    yajl_val content = yajl_tree_parse(msg, error_buf, sizeof(error_buf));
    if (content == NULL) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: on msg: parse msg fail, error=%s\n%s!",
            tester->m_id, error_buf, msg);
        return;
    }

    struct net_ndt7_measurement measurement;
    net_ndt7_measurement_from_json(&measurement, content, manager->m_em);
    yajl_tree_free(content);

    net_ndt7_tester_notify_measurement_progress(tester, &measurement);
}

static void net_ndt7_tester_download_on_msg_bin(void * ctx, net_ws_endpoint_t endpoin, const void * msg, uint32_t msg_len) {
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    tester->m_download.m_num_bytes += msg_len;
    net_ndt7_tester_download_try_notify_update(tester);
}

static void net_ndt7_tester_download_on_close(
    void * ctx, net_ws_endpoint_t endpoin, uint16_t status_code, const void * msg, uint32_t msg_len)
{
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    if (status_code == net_ws_status_code_normal_closure) {
        int64_t cur_time_ms = net_schedule_cur_time_ms(manager->m_schedule);

        struct net_ndt7_response response;
        net_ndt7_response_init(
            &response, tester->m_download.m_start_time_ms, cur_time_ms, tester->m_download.m_num_bytes,
            net_ndt7_test_download);

        net_ndt7_tester_notify_test_complete(
            tester,
            net_endpoint_error_source_none, 0, NULL,
            &response, net_ndt7_test_download);
        tester->m_download.m_completed = 1;
    }
}

static void net_ndt7_tester_download_on_endpoint_fini(void * ctx, net_endpoint_t endpoint) {
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    assert(tester->m_state == net_ndt7_tester_state_download);
    assert(tester->m_download.m_endpoint);
    assert(net_ws_endpoint_base_endpoint(tester->m_download.m_endpoint) == endpoint);

    tester->m_download.m_endpoint = NULL;

    net_ndt7_tester_check_start_next_step(tester);
}

static void net_ndt7_tester_download_on_endpoint_evt(
    void * ctx, net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt)
{
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    if (evt->m_type == net_endpoint_monitor_evt_state_changed) {
        switch(net_endpoint_state(endpoint)) {
        case net_endpoint_state_disable:
        case net_endpoint_state_error:
            if (!tester->m_download.m_completed) {
                int64_t cur_time_ms = net_schedule_cur_time_ms(manager->m_schedule);

                struct net_ndt7_response response;
                net_ndt7_response_init(
                    &response, tester->m_download.m_start_time_ms, cur_time_ms, tester->m_download.m_num_bytes,
                    net_ndt7_test_download);

                if (net_endpoint_state(endpoint) == net_endpoint_state_disable) {
                    net_ndt7_tester_notify_test_complete(
                        tester,
                        net_endpoint_error_source_network,
                        net_endpoint_network_errno_internal,
                        "Disabled",
                        &response, net_ndt7_test_download);
                } else {
                    net_ndt7_tester_notify_test_complete(
                        tester,
                        net_endpoint_error_source(endpoint),
                        net_endpoint_error_no(endpoint),
                        net_endpoint_error_msg(endpoint),
                        &response, net_ndt7_test_download);
                }
                tester->m_download.m_completed = 1;
            }
            break;
        default:
            break;
        }
    }
}

static void net_ndt7_tester_download_try_notify_update(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;
    int64_t cur_time_ms = net_schedule_cur_time_ms(manager->m_schedule);

    if (cur_time_ms - tester->m_download.m_pre_notify_ms >= tester->m_cfg.m_measurement_interval_ms) {
        struct net_ndt7_response response;
        net_ndt7_response_init(
            &response, tester->m_download.m_start_time_ms, cur_time_ms, tester->m_download.m_num_bytes,
            net_ndt7_test_download);
        net_ndt7_tester_notify_speed_progress(tester, &response);
        tester->m_download.m_pre_notify_ms = cur_time_ms;
    }
}
