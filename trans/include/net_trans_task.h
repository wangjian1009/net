#ifndef NET_TRANS_TASK_H_INCLEDED
#define NET_TRANS_TASK_H_INCLEDED
#include "net_trans_system.h"

NET_BEGIN_DECL

net_trans_task_t net_trans_task_create(net_trans_manage_t mgr, net_trans_method_t method, const char * url, uint8_t is_debug);

void net_trans_task_free(net_trans_task_t task);

uint32_t net_trans_task_id(net_trans_task_t task);
net_trans_task_t net_trans_task_find(net_trans_manage_t mgr, uint32_t task_id);
net_trans_manage_t net_trans_task_manage(net_trans_task_t task);
net_address_t net_trans_task_target_address(net_trans_task_t task);

void net_trans_task_set_debug(net_trans_task_t task, uint8_t is_debug);

/*req*/
int net_trans_task_set_timeout_ms(net_trans_task_t task, uint64_t timeout_ms);
int net_trans_task_set_connection_timeout_ms(net_trans_task_t task, uint64_t timeout_ms);
int net_trans_task_append_header(net_trans_task_t task, const char * name, const char * value);
int net_trans_task_append_header_line(net_trans_task_t task, const char * head_one);
int net_trans_task_set_body(net_trans_task_t task, void const * data, uint32_t data_size);
int net_trans_task_set_user_agent(net_trans_task_t task, const char * user_agent);
int net_trans_task_set_net_interface(net_trans_task_t task, const char * net_interface);
int net_trans_task_set_protect_vpn(net_trans_task_t task, uint8_t protect_vpn);
int net_trans_task_set_follow_location(net_trans_task_t task, uint8_t enable);
int net_trans_task_set_proxy_http_1_1(net_trans_task_t task, net_address_t address);

void net_trans_task_clear_callback(net_trans_task_t task);
void net_trans_task_set_callback(
    net_trans_task_t task,
    net_trans_task_commit_op_t commit,
    net_trans_task_progress_op_t progress,
    net_trans_task_write_op_t write,
    net_trans_task_head_op_t head,
    void * ctx, void (*ctx_free)(void *));

int net_trans_task_start(net_trans_task_t task);

/*res*/
net_trans_task_state_t net_trans_task_state(net_trans_task_t task);
net_trans_task_result_t net_trans_task_result(net_trans_task_t task);
net_trans_task_error_t net_trans_task_error(net_trans_task_t task);
const char * net_trans_task_error_addition(net_trans_task_t task);

int16_t net_trans_task_res_code(net_trans_task_t task);
const char * net_trans_task_res_mine(net_trans_task_t task);
const char * net_trans_task_res_charset(net_trans_task_t task);

/*statistics*/

/*cost
    begin
    |
    |--NAMELOOKUP
    |--|--CONNECT
    |--|--|--APPCONNECT
    |--|--|--|--PRETRANSFER
    |--|--|--|--|--STARTTRANSFER
    |--|--|--|--|--|--TOTAL
    |--|--|--|--|--|--REDIRECT
*/
struct net_trans_task_cost_info {
    int32_t dns_ms;
    int32_t connect_ms;
    int32_t app_connect_ms;
    int32_t pre_transfer_ms;
    int32_t start_transfer_ms;
    int32_t total_ms;
    int32_t redirect_ms;
};
int net_trans_task_cost_info(net_trans_task_t task, net_trans_task_cost_info_t cost_info);

/*utils*/
const char * net_trans_task_state_str(net_trans_task_state_t state);
const char * net_trans_task_result_str(net_trans_task_result_t result);
const char * net_trans_task_error_str(net_trans_task_error_t err);
int net_trans_task_error_from_str(const char * err, net_trans_task_error_t * r);

NET_END_DECL

#endif
