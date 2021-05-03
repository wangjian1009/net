#ifndef NET_NDT7_TESTER_H_INCLEDED
#define NET_NDT7_TESTER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ndt7_system.h"

NET_BEGIN_DECL

enum net_ndt7_test_type {
    net_ndt7_test_upload,
    net_ndt7_test_download,
    net_ndt7_test_download_and_upload,
};

enum net_ndt7_test_protocol {
    net_ndt7_test_protocol_auto,
    net_ndt7_test_protocol_ws,
    net_ndt7_test_protocol_wss,
};

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

void net_ndt7_tester_targets(net_ndt7_tester_t tester, net_ndt7_tester_target_it_t it);

/*回调 */
typedef void (*net_ndt7_tester_on_complete_fun_t)(void * ctx, net_ndt7_tester_t tester);

void net_ndt7_tester_set_cb(
    net_ndt7_tester_t tester,
    void * ctx,
    net_ndt7_tester_on_complete_fun_t on_complete,
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
const char * net_ndt7_test_type_str(net_ndt7_test_type_t state);
const char * net_ndt7_tester_state_str(net_ndt7_tester_state_t state);
const char * net_ndt7_tester_error_str(net_ndt7_tester_error_t err);

NET_END_DECL

#endif
