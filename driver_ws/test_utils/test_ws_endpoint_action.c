#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "net_endpoint.h"
#include "test_net_tl_op.h"
#include "test_ws_endpoint_action.h"

void test_net_ws_endpoint_apply_action_i(net_ws_endpoint_t endpoint, struct test_net_ws_endpoint_action * action) {
    switch(action->m_type) {
    case test_net_ws_endpoint_op_noop:
        break;
    case test_net_ws_endpoint_op_disable:
        if (net_endpoint_set_state(net_ws_endpoint_base_endpoint(endpoint), net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(net_ws_endpoint_base_endpoint(endpoint), net_endpoint_state_deleting);
        }
        break;
    case test_net_ws_endpoint_op_error:
        if (net_endpoint_set_state(net_ws_endpoint_base_endpoint(endpoint), net_endpoint_state_error) != 0) {
            net_endpoint_set_state(net_ws_endpoint_base_endpoint(endpoint), net_endpoint_state_deleting);
        }
        break;
    case test_net_ws_endpoint_op_delete:
        net_endpoint_set_state(net_ws_endpoint_base_endpoint(endpoint), net_endpoint_state_deleting);
        break;
    case test_net_ws_endpoint_op_close:
        net_ws_endpoint_close(
            endpoint,
            action->m_close.m_status_code,
            action->m_close.m_msg,
            action->m_close.m_msg ? strlen(action->m_close.m_msg) : 0);
        break;
    case test_net_ws_endpoint_op_bin_msg:
        net_ws_endpoint_send_msg_bin(endpoint, action->m_bin_msg.m_msg, action->m_bin_msg.m_msg_size);
        break;
    case test_net_ws_endpoint_op_text_msg:
        net_ws_endpoint_send_msg_text(endpoint, action->m_text_msg.m_msg);
        break;
    }
}

struct test_net_ws_endpoint_op_data {
    uint32_t m_ep_id;
    struct test_net_ws_endpoint_action m_action;
};

static void test_net_ws_endpoint_op_cb(void * ctx, test_net_tl_op_t op) {
    struct test_net_ws_endpoint_op_data * op_data = test_net_tl_op_data(op);

    assert_true(op_data->m_ep_id != 0);

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(op->m_driver));
    net_endpoint_t base_endpoint = net_endpoint_find(schedule, op_data->m_ep_id);
    if (base_endpoint == NULL) return;

    net_ws_endpoint_t endpoint = net_ws_endpoint_cast(base_endpoint);
    
    test_net_ws_endpoint_apply_action_i(endpoint, &op_data->m_action);
}

void test_net_ws_endpoint_apply_action(
    test_net_driver_t tdriver, net_ws_endpoint_t endpoint, test_net_ws_endpoint_action_t action, int64_t delay_ms)
{
    if (delay_ms == 0) {
        test_net_ws_endpoint_apply_action_i(endpoint, action);
    }
    else {
        test_net_tl_op_t op = test_net_tl_op_create(
            tdriver, delay_ms,
            sizeof(struct test_net_ws_endpoint_op_data),
            test_net_ws_endpoint_op_cb, NULL, NULL);

        struct test_net_ws_endpoint_op_data * op_data = test_net_tl_op_data(op);

        op_data->m_ep_id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
        op_data->m_action = *action;
    }
}

void test_net_ws_endpoint_action_copy(
    test_net_driver_t tdriver, test_net_ws_endpoint_action_t to, test_net_ws_endpoint_action_t from)
{
    *to = *from;

    switch(to->m_type) {
    case test_net_ws_endpoint_op_noop:
    case test_net_ws_endpoint_op_disable:
    case test_net_ws_endpoint_op_error:
    case test_net_ws_endpoint_op_delete:
        break;
    case test_net_ws_endpoint_op_close:
        if (from->m_close.m_msg) {
            to->m_close.m_msg = mem_buffer_strdup(&tdriver->m_setup_buffer, from->m_close.m_msg);
        }
        break;
    case test_net_ws_endpoint_op_bin_msg:
        if (from->m_bin_msg.m_msg) {
            to->m_bin_msg.m_msg = mem_buffer_alloc(&tdriver->m_setup_buffer, from->m_bin_msg.m_msg_size);
            memcpy((void*)to->m_bin_msg.m_msg, from->m_bin_msg.m_msg, from->m_bin_msg.m_msg_size);
        }
        break;
    case test_net_ws_endpoint_op_text_msg:
        if (from->m_text_msg.m_msg) {
            to->m_text_msg.m_msg = mem_buffer_strdup(&tdriver->m_setup_buffer, from->m_text_msg.m_msg);
        }
        break;
    }
}
