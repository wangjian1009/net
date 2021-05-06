#include <assert.h>
#include "yajl/yajl_tree.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/url.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_ws_endpoint.h"
#include "net_ndt7_tester_i.h"
#include "net_ndt7_model_i.h"
#include "net_ndt7_json_i.h"

static void net_ndt7_tester_download_on_msg_text(void * ctx, net_ws_endpoint_t endpoin, const char * msg);
static void net_ndt7_tester_download_on_msg_bin(void * ctx, net_ws_endpoint_t endpoin, const void * msg, uint32_t msg_len);
static void net_ndt7_tester_download_on_endpoint_fini(void * ctx, net_endpoint_t endpoint);
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
    net_endpoint_set_data_watcher(
        base_endpoint,
        tester,
        NULL,
        net_ndt7_tester_download_on_endpoint_fini);

    tester->m_download.m_endpoint = net_ws_endpoint_cast(base_endpoint);
    assert(tester->m_download.m_endpoint);

    net_ws_endpoint_set_msg_receiver_text(
        tester->m_download.m_endpoint, tester, net_ndt7_tester_download_on_msg_text, NULL);

    net_ws_endpoint_set_msg_receiver_bin(
        tester->m_download.m_endpoint, tester, net_ndt7_tester_download_on_msg_bin, NULL);
    
    net_address_t remote_address = net_address_create_from_url(manager->m_schedule, tester->m_download_url);
    if (remote_address == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: download: start: create protocol fail", tester->m_id);
        goto START_FAIL;
    }

    if (net_endpoint_set_remote_address(base_endpoint, remote_address) != 0) {
        CPE_ERROR(manager->m_em, "ndt7: %d: download: start: set remote address fail", tester->m_id);
        net_address_free(remote_address);
        goto START_FAIL;
    }
    net_address_free(remote_address);

    if (net_endpoint_connect(base_endpoint) != 0) {
        CPE_ERROR(manager->m_em, "ndt7: %d: download: start: connect start fail", tester->m_id);
        goto START_FAIL;
    }
    
    return 0;

START_FAIL:
    if (tester->m_download.m_endpoint) {
        net_endpoint_set_data_watcher(base_endpoint, NULL, NULL, NULL);
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
    tester->m_download.m_num_bytes += msg_len;
    net_ndt7_tester_download_try_notify_update(tester);
}

static void net_ndt7_tester_download_on_endpoint_fini(void * ctx, net_endpoint_t endpoint) {
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    assert(tester->m_state == net_ndt7_tester_state_query_target);
    assert(tester->m_query_target.m_endpoint);
    assert(net_ws_endpoint_base_endpoint(tester->m_download.m_endpoint) == endpoint);

    tester->m_download.m_endpoint = NULL;
}

static void net_ndt7_tester_download_try_notify_update(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;
    int64_t cur_time_ms = net_schedule_cur_time_ms(manager->m_schedule);

    /* if we haven't sent an update in 250ms, lets send one */
    if (cur_time_ms - tester->m_download.m_pre_notify_ms > MEASUREMENT_INTERVAL_MS) {
        struct net_ndt7_response response;
        net_ndt7_response_init(
            &response, tester->m_download.m_start_time_ms, cur_time_ms, tester->m_download.m_num_bytes,
            net_ndt7_test_download);
        net_ndt7_tester_notify_speed_progress(tester, &response);
        tester->m_download.m_pre_notify_ms = cur_time_ms;
    }
}
