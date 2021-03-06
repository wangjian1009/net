#ifndef NET_NDT7_TESTER_H_INCLEDED
#define NET_NDT7_TESTER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ndt7_system.h"

NET_BEGIN_DECL

enum net_ndt7_tester_state {
    net_ndt7_tester_state_init,
    net_ndt7_tester_state_query_target,
    net_ndt7_tester_state_download,
    net_ndt7_tester_state_upload,
    net_ndt7_tester_state_done,
};

net_ndt7_tester_t
net_ndt7_tester_create(net_ndt7_manage_t manager);

void net_ndt7_tester_free(net_ndt7_tester_t tester);

net_ndt7_test_type_t net_ndt7_tester_type(net_ndt7_tester_t tester);
int net_ndt7_tester_set_type(net_ndt7_tester_t tester, net_ndt7_test_type_t test_type);

net_ndt7_test_protocol_t net_ndt7_tester_protocol(net_ndt7_tester_t tester);
int net_ndt7_tester_set_protocol(net_ndt7_tester_t tester, net_ndt7_test_protocol_t protocol);

net_ndt7_tester_state_t net_ndt7_tester_state(net_ndt7_tester_t tester);

net_ndt7_config_t net_ndt7_tester_config(net_ndt7_tester_t tester);
void net_ndt7_tester_set_config(net_ndt7_tester_t tester, net_ndt7_config_t config);

const char * net_ndt7_tester_effect_machine(net_ndt7_tester_t tester);
const char * net_ndt7_tester_effect_country(net_ndt7_tester_t tester);
const char * net_ndt7_tester_effect_city(net_ndt7_tester_t tester);
net_ndt7_test_protocol_t net_ndt7_tester_effect_protocol(net_ndt7_tester_t tester);

int net_ndt7_tester_set_target(net_ndt7_tester_t tester, cpe_url_t url);
void net_ndt7_tester_targets(net_ndt7_tester_t tester, net_ndt7_tester_target_it_t it);

/*回调 */
typedef void (*net_ndt7_tester_on_speed_progress_fun_t)(
    void * ctx, net_ndt7_tester_t tester, net_ndt7_response_t response);

typedef void (*net_ndt7_tester_on_measurement_progress_fun_t)(
    void * ctx, net_ndt7_tester_t tester, net_ndt7_measurement_t response);

typedef void (*net_ndt7_tester_on_test_complete_fun_t)(
    void * ctx, net_ndt7_tester_t tester,
    net_endpoint_error_source_t error_source, int error_code, const char * error_msg,
    net_ndt7_response_t response, net_ndt7_test_type_t test_type);

typedef void (*net_ndt7_tester_on_all_complete_fun_t)(void * ctx, net_ndt7_tester_t tester);

void net_ndt7_tester_set_cb(
    net_ndt7_tester_t tester,
    void * ctx,
    net_ndt7_tester_on_speed_progress_fun_t on_speed_progress,
    net_ndt7_tester_on_measurement_progress_fun_t on_measurement_progress,
    net_ndt7_tester_on_test_complete_fun_t on_test_complete,
    net_ndt7_tester_on_all_complete_fun_t on_all_complete,
    void (*ctx_free)(void *));
    
void net_ndt7_tester_clear_cb(net_ndt7_tester_t tester); 

int net_ndt7_tester_start(net_ndt7_tester_t tester);

/*错误信息 */
enum net_ndt7_tester_error {
    net_ndt7_tester_error_none,
    net_ndt7_tester_error_timeout,
    net_ndt7_tester_error_cancel,
    net_ndt7_tester_error_network,
    net_ndt7_tester_error_internal = -1,
};

net_ndt7_tester_state_t net_ndt7_tester_error_state(net_ndt7_tester_t tester);
net_ndt7_tester_error_t net_ndt7_tester_error(net_ndt7_tester_t tester);
const char * net_ndt7_tester_error_msg(net_ndt7_tester_t tester);

/*辅助函数 */
const char * net_ndt7_tester_state_str(net_ndt7_tester_state_t state);
const char * net_ndt7_tester_error_str(net_ndt7_tester_error_t err);

NET_END_DECL

#endif
