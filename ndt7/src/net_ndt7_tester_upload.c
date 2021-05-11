#include <assert.h>
#include "yajl/yajl_tree.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/url.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_endpoint_monitor.h"
#include "net_protocol.h"
#include "net_timer.h"
#include "net_ws_endpoint.h"
#include "net_ndt7_tester_i.h"
#include "net_ndt7_tester_i.h"
#include "net_ndt7_model_i.h"
#include "net_ndt7_json_i.h"

static void net_ndt7_tester_upload_on_connected(void * ctx, net_ws_endpoint_t endpoin);
static void net_ndt7_tester_upload_on_msg_text(void * ctx, net_ws_endpoint_t endpoin, const char * msg);
static void net_ndt7_tester_upload_on_close(
    void * ctx, net_ws_endpoint_t endpoin, uint16_t status_code, const void * msg, uint32_t msg_len);

static void net_ndt7_tester_upload_tick(net_ndt7_tester_t tester);
static void net_ndt7_tester_upload_on_process(net_timer_t timer, void * ctx);

static void net_ndt7_tester_upload_on_endpoint_fini(void * ctx, net_endpoint_t endpoint);
static void net_ndt7_tester_upload_on_endpoint_evt(
    void * ctx, net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt);

static double net_ndt7_tester_upload_total_bytes(net_ndt7_tester_t tester);
static double net_ndt7_tester_upload_total_bytes_transmitted(net_ndt7_tester_t tester, double queue_size);

int net_ndt7_tester_upload_start(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    assert(tester->m_state == net_ndt7_tester_state_upload);

    if (tester->m_upload_url == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: upload: start: no upload url", tester->m_id);
        net_ndt7_tester_set_error_internal(tester, "NoDownloadUrl");
        return -1;
    }

    assert(tester->m_upload.m_process_timer == NULL);
    if (tester->m_upload.m_process_timer == NULL) {
        tester->m_upload.m_process_timer =
            net_timer_auto_create(manager->m_schedule, net_ndt7_tester_upload_on_process, tester);
        if (tester->m_upload.m_process_timer == NULL) {
            CPE_ERROR(manager->m_em, "ndt7: %d: upload: start: create stop timer fail", tester->m_id);
            net_ndt7_tester_set_error_internal(tester, "CreateStopTimerError");
            return -1;
        }
    }

    net_driver_t driver = NULL;
    if (strcasecmp(cpe_url_protocol(tester->m_upload_url), "ws") == 0) {
        driver = manager->m_base_driver;
    }
    else if (strcasecmp(cpe_url_protocol(tester->m_upload_url), "wss") == 0) {
        driver = manager->m_ssl_driver;
    }
    else {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: upload: start: not support protocol %s",
            tester->m_id, cpe_url_protocol(tester->m_upload_url));
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "NotSupportProtocol(%s)", cpe_url_protocol(tester->m_upload_url));
        net_ndt7_tester_set_error_internal(tester, error_msg);
        return -1;
    }
    
    net_endpoint_t base_endpoint =
        net_endpoint_create(driver, net_protocol_from_data(manager->m_ws_protocol), NULL);
    if (base_endpoint == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: upload: start: create protocol fail", tester->m_id);
        net_ndt7_tester_set_error_internal(tester, "CreateEndpointFail");
        return -1;
    }
    tester->m_upload.m_endpoint = net_ws_endpoint_cast(base_endpoint);
    assert(tester->m_upload.m_endpoint);
    /* net_endpoint_set_protocol_debug(base_endpoint, 2); */
    /* net_endpoint_set_driver_debug(base_endpoint, 2); */
    net_endpoint_set_auto_free(base_endpoint, 1);

    if (net_endpoint_monitor_create(base_endpoint, tester, NULL, net_ndt7_tester_upload_on_endpoint_evt) == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: upload: art: create state monitor fail!", tester->m_id);
        net_ndt7_tester_set_error_internal(tester, "CreateEndpointMonitorFail");
        goto START_FAIL;
    }
    
    net_endpoint_set_data_watcher(
        base_endpoint,
        tester,
        NULL,
        net_ndt7_tester_upload_on_endpoint_fini);

    net_ws_endpoint_header_add(
        tester->m_upload.m_endpoint,
        "Sec-WebSocket-Protocol", "net.measurementlab.ndt.v7");
    
    net_ws_endpoint_set_callback(
        tester->m_upload.m_endpoint,
        tester,
        net_ndt7_tester_upload_on_connected,
        net_ndt7_tester_upload_on_msg_text,
        NULL,
        net_ndt7_tester_upload_on_close,
        NULL);

    if (net_ws_endpoint_connect(tester->m_upload.m_endpoint, tester->m_upload_url) != 0) {
        CPE_ERROR(manager->m_em, "ndt7: %d: upload: start: connect start fail", tester->m_id);
        goto START_FAIL;
    }

    tester->m_upload.m_start_time_ms = net_schedule_cur_time_ms(manager->m_schedule);
    tester->m_upload.m_pre_notify_ms = tester->m_upload.m_start_time_ms;
    tester->m_upload.m_package_size = tester->m_cfg.m_upload.m_min_message_size;
    net_timer_active(tester->m_upload.m_process_timer, 0);
    return 0;

START_FAIL:
    if(tester->m_upload.m_process_timer) {
        net_timer_free(tester->m_upload.m_process_timer);
        tester->m_upload.m_process_timer = NULL;
    }

    if (tester->m_upload.m_endpoint) {
        net_endpoint_set_data_watcher(base_endpoint, NULL, NULL, NULL);
        net_endpoint_monitor_free_by_ctx(base_endpoint, tester);
        net_ws_endpoint_free(tester->m_upload.m_endpoint);
        tester->m_upload.m_endpoint = NULL;
    }
    
    return -1;
}

static void net_ndt7_tester_upload_on_connected(void * ctx, net_ws_endpoint_t endpoin) {
    net_ndt7_tester_t tester = ctx;
    net_ndt7_tester_upload_tick(tester);    
}

static void net_ndt7_tester_upload_on_msg_text(void * ctx, net_ws_endpoint_t endpoin, const char * msg) {
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

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

static void net_ndt7_tester_upload_on_close(
    void * ctx, net_ws_endpoint_t endpoin, uint16_t status_code, const void * msg, uint32_t msg_len)
{
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    if (status_code == net_ws_status_code_normal_closure) {
        int64_t cur_time_ms = net_schedule_cur_time_ms(manager->m_schedule);

        struct net_ndt7_response response;
        net_ndt7_response_init(
            &response, tester->m_upload.m_start_time_ms, cur_time_ms, net_ndt7_tester_upload_total_bytes(tester),
            net_ndt7_test_upload);

        net_ndt7_tester_notify_test_complete(
            tester,
            net_endpoint_error_source_none, 0, NULL,
            &response, net_ndt7_test_upload);

        tester->m_upload.m_completed = 1;
    }

    if (tester->m_upload.m_process_timer) {
        net_timer_free(tester->m_upload.m_process_timer);
        tester->m_upload.m_process_timer = NULL;
    }
}

// this is gonna let higher speed clients saturate their pipes better
// it will gradually increase the size of data if the websocket queue isn't filling up
static void net_ndt7_tester_upload_perform_dynamic_tuning(net_ndt7_tester_t tester, uint32_t queue_size) {
    uint64_t total_bytes_transmitted = tester->m_upload.m_total_bytes_queued - queue_size;

    if (tester->m_upload.m_package_size * 2 < tester->m_cfg.m_upload.m_max_message_size
        && tester->m_upload.m_package_size < total_bytes_transmitted / 16)
    {
        tester->m_upload.m_package_size *= 2;
    }
}

static void net_ndt7_tester_upload_send(net_ndt7_tester_t tester, uint32_t queue_size) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (queue_size + tester->m_upload.m_package_size < tester->m_cfg.m_upload.m_max_queue_size) {
        void * buf = mem_buffer_alloc(net_ndt7_manage_tmp_buffer(manager), tester->m_upload.m_package_size);
        if (buf == NULL) {
            CPE_ERROR(
                manager->m_em, "ndt7: %d: upload: send: alloc buffer error, size=%d",
                tester->m_id, tester->m_upload.m_package_size);
            return;
        }

        if (net_ws_endpoint_send_msg_bin(tester->m_upload.m_endpoint, buf, tester->m_upload.m_package_size) != 0) {
            CPE_ERROR(
                manager->m_em, "ndt7: %d: upload: send: send error, size=%d",
                tester->m_id, tester->m_upload.m_package_size);
            return;
        }
        
        tester->m_upload.m_total_bytes_queued += tester->m_upload.m_package_size;
    }
}

static void net_ndt7_tester_upload_tick(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    assert(tester->m_state == net_ndt7_tester_state_upload);
    assert(tester->m_upload.m_endpoint);

    int64_t complete_time_ms = tester->m_upload.m_start_time_ms + tester->m_cfg.m_upload.m_duration_ms;
    int64_t cur_time_ms = net_schedule_cur_time_ms(manager->m_schedule);
    if (cur_time_ms >= complete_time_ms) {
        if (net_ws_endpoint_close(tester->m_upload.m_endpoint, net_ws_status_code_normal_closure, NULL, 0) != 0) {
            net_endpoint_set_state(
                net_ws_endpoint_base_endpoint(tester->m_upload.m_endpoint), net_endpoint_state_deleting);
        }
        return;
    }

    assert(tester->m_upload.m_endpoint);

    if (net_ws_endpoint_state(tester->m_upload.m_endpoint) == net_ws_endpoint_state_streaming) {
        net_endpoint_t base_endpoint = net_ws_endpoint_base_endpoint(tester->m_upload.m_endpoint);

        //endpoint->m_state != net_ws_endpoint_state_streaming
        //if (net_endpoint_state(base_endpoint) == net_endpoint_state_established
        struct net_endpoint_size_info size_info = { 0 };
        net_endpoint_calc_size(base_endpoint, &size_info);

        /*更新发送包大小 */
        net_ndt7_tester_upload_perform_dynamic_tuning(tester, size_info.m_write);

        /*发送数据 */
        net_ndt7_tester_upload_send(tester, size_info.m_write);
    }

    /*指定间隔发送进度 */
    if (tester->m_cfg.m_progress_interval_ms > 0
        && cur_time_ms - tester->m_upload.m_pre_notify_ms >= tester->m_cfg.m_progress_interval_ms)
    {
        struct net_ndt7_response response;
        net_ndt7_response_init(
            &response, tester->m_upload.m_start_time_ms, cur_time_ms, net_ndt7_tester_upload_total_bytes(tester),
            net_ndt7_test_upload);
        net_ndt7_tester_notify_speed_progress(tester, &response);
        tester->m_upload.m_pre_notify_ms = cur_time_ms;
    }

    /*计算下一次触发时间 */
    int64_t delay_ms = complete_time_ms - cur_time_ms;
    if (delay_ms > 10) {
        delay_ms = 10;
    }

    net_timer_active(tester->m_upload.m_process_timer, delay_ms);
}

static void net_ndt7_tester_upload_on_process(net_timer_t timer, void * ctx) {
    net_ndt7_tester_upload_tick(ctx);
}

static void net_ndt7_tester_upload_on_endpoint_fini(void * ctx, net_endpoint_t endpoint) {
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    assert(tester->m_state == net_ndt7_tester_state_upload);
    assert(tester->m_upload.m_endpoint);
    assert(net_ws_endpoint_base_endpoint(tester->m_upload.m_endpoint) == endpoint);

    tester->m_upload.m_endpoint = NULL;

    if (tester->m_upload.m_process_timer) {
        net_timer_free(tester->m_upload.m_process_timer);
        tester->m_upload.m_process_timer = NULL;
    }
    
    net_ndt7_tester_check_start_next_step(tester);
}

static void net_ndt7_tester_upload_on_endpoint_evt(
    void * ctx, net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt)
{
    net_ndt7_tester_t tester = ctx;
    net_ndt7_manage_t manager = tester->m_manager;

    if (evt->m_type == net_endpoint_monitor_evt_state_changed) {
        switch(net_endpoint_state(endpoint)) {
        case net_endpoint_state_disable:
        case net_endpoint_state_error:
            if (tester->m_upload.m_process_timer) {
                net_timer_free(tester->m_upload.m_process_timer);
                tester->m_upload.m_process_timer = NULL;
            }
            
            if (!tester->m_upload.m_completed) {
                int64_t cur_time_ms = net_schedule_cur_time_ms(manager->m_schedule);

                struct net_ndt7_response response;
                net_ndt7_response_init(
                    &response, tester->m_upload.m_start_time_ms, cur_time_ms, net_ndt7_tester_upload_total_bytes(tester),
                    net_ndt7_test_upload);

                if (net_endpoint_state(endpoint) == net_endpoint_state_disable) {
                    net_ndt7_tester_notify_test_complete(
                        tester,
                        net_endpoint_error_source_network,
                        net_endpoint_network_errno_internal,
                        "Disabled",
                        &response, net_ndt7_test_upload);
                } else {
                    net_ndt7_tester_notify_test_complete(
                        tester,
                        net_endpoint_error_source(endpoint),
                        net_endpoint_error_no(endpoint),
                        net_endpoint_error_msg(endpoint),
                        &response, net_ndt7_test_upload);
                }
                tester->m_upload.m_completed = 1;
            }
            break;
        default:
            break;
        }
    }
}

static double net_ndt7_tester_upload_total_bytes(net_ndt7_tester_t tester) {
    struct net_endpoint_size_info size_info = { 0 };
    if (tester->m_upload.m_endpoint) {
        net_endpoint_calc_size(net_ws_endpoint_base_endpoint(tester->m_upload.m_endpoint), &size_info);
    }
    
    return net_ndt7_tester_upload_total_bytes_transmitted(tester, size_info.m_write);
}

static double net_ndt7_tester_upload_total_bytes_transmitted(net_ndt7_tester_t tester, double queue_size) {
    return tester->m_upload.m_total_bytes_queued > queue_size
        ? tester->m_upload.m_total_bytes_queued - queue_size
        : 0;
}

        /* return if (data.size * 2 < NDT7Constants.MAX_MESSAGE_SIZE && data.size < totalBytesTransmitted / 16) { */
        /*     ByteString.of(*ByteArray(data.size * 2)) // double the size of data */
        /* } else { */
        /*     data */
        /* } */
