#ifndef NET_NDT7_TESTER_I_H_INCLEDED
#define NET_NDT7_TESTER_I_H_INCLEDED
#include "net_ndt7_model.h"
#include "net_ndt7_config.h"
#include "net_ndt7_tester.h"
#include "net_ndt7_manage_i.h"

NET_BEGIN_DECL

struct net_ndt7_tester {
    net_ndt7_manage_t m_manager;
    uint32_t m_id;
    TAILQ_ENTRY(net_ndt7_tester) m_next;

    net_ndt7_test_type_t m_type;
    net_ndt7_test_protocol_t m_protocol;
    net_ndt7_tester_state_t m_state;
    uint8_t m_is_processing;
    uint8_t m_is_free;

    struct net_ndt7_config m_cfg;

    uint8_t m_is_to_notify;
    TAILQ_ENTRY(net_ndt7_tester) m_next_for_notify;
    
    /*状态数据 */
    net_ndt7_tester_target_t m_target;
    cpe_url_t m_upload_url;
    cpe_url_t m_download_url;
    net_ndt7_tester_target_list_t m_targets;

    struct {
        net_http_endpoint_t m_endpoint;
        net_http_req_t m_req;
    } m_query_target;
    
    struct {
        int64_t m_start_time_ms;
        int64_t m_pre_notify_ms;
        uint64_t m_num_bytes;
        uint8_t m_completed;
        net_ws_endpoint_t m_endpoint;
    } m_download;

    struct {
        int64_t m_start_time_ms;
        int64_t m_pre_notify_ms;
        uint64_t m_total_bytes_queued;
        uint32_t m_package_size;
        uint8_t m_completed;
        net_ws_endpoint_t m_endpoint;
        net_timer_t m_process_timer;
    } m_upload;
    
    /*错误数据 */
    struct {
        net_ndt7_tester_state_t m_state;
        net_ndt7_tester_error_t m_error;
        char * m_msg;
    } m_error;
    
    /*callback*/
    void * m_ctx;
    net_ndt7_tester_on_speed_progress_fun_t m_on_speed_progress;
    net_ndt7_tester_on_measurement_progress_fun_t m_on_measurement_progress;
    net_ndt7_tester_on_test_complete_fun_t m_on_test_complete;
    net_ndt7_tester_on_all_complete_fun_t m_on_all_complete;
    void (*m_ctx_free)(void *);
};

int net_ndt7_tester_query_target_start(net_ndt7_tester_t tester);
int net_ndt7_tester_download_start(net_ndt7_tester_t tester);
int net_ndt7_tester_upload_start(net_ndt7_tester_t tester);

void net_ndt7_tester_set_error(
    net_ndt7_tester_t tester, net_ndt7_tester_error_t err, const char * msg);
void net_ndt7_tester_set_error_internal(net_ndt7_tester_t tester, const char * msg);

int net_ndt7_tester_check_start_next_step(net_ndt7_tester_t tester);
void net_ndt7_tester_notify_complete(net_ndt7_tester_t tester);

void net_ndt7_tester_notify_speed_progress(net_ndt7_tester_t tester, net_ndt7_response_t response);
void net_ndt7_tester_notify_measurement_progress(net_ndt7_tester_t tester, net_ndt7_measurement_t measurement);

void net_ndt7_tester_notify_test_complete(
    net_ndt7_tester_t tester,
    net_endpoint_error_source_t error_source, int error_code, const char * error_msg,
    net_ndt7_response_t response, net_ndt7_test_type_t test_type);

NET_END_DECL

#endif
