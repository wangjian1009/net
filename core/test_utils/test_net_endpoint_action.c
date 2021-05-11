#include "cmocka_all.h"
#include "test_net_tl_op.h"
#include "test_net_endpoint_action.h"

void test_net_endpoint_apply_action_i(net_endpoint_t endpoint, test_net_endpoint_action_t action) {
    switch(action->m_type) {
    case test_net_endpoint_action_noop:
        break;
    case test_net_endpoint_action_buf_copy:
        assert_return_code(
            net_endpoint_buf_append_from_self(
                endpoint, action->m_buf_copy.m_buf_type,
                net_ep_buf_write, 0),
            0);
        break;
    case test_net_endpoint_action_buf_erase:
        break;
    case test_net_endpoint_action_buf_link:
        break;
    case test_net_endpoint_action_disable:
        if (net_endpoint_set_state(endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
        }
        break;
    case test_net_endpoint_action_error:
        if (net_endpoint_set_state(endpoint, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
        }
        break;
    case test_net_endpoint_action_delete:
        net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
        break;
    }
}

struct test_net_endpoint_op_data {
    uint32_t m_ep_id;
    struct test_net_endpoint_action m_action;
};

static void test_net_endpoint_op_cb(void * ctx, test_net_tl_op_t op) {
    struct test_net_endpoint_op_data * op_data = test_net_tl_op_data(op);

    assert_true(op_data->m_ep_id != 0);

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(op->m_driver));
    net_endpoint_t endpoint = net_endpoint_find(schedule, op_data->m_ep_id);
    if (endpoint == NULL) return;

    test_net_endpoint_apply_action_i(endpoint, &op_data->m_action);
}

void test_net_endpoint_apply_action(
    test_net_driver_t tdriver, net_endpoint_t endpoint,
    test_net_endpoint_action_t action, int64_t delay_ms)
{
    if (delay_ms == 0) {
        test_net_endpoint_apply_action_i(endpoint, action);
    }
    else {
        test_net_tl_op_t op = test_net_tl_op_create(
            tdriver, delay_ms,
            sizeof(struct test_net_endpoint_op_data),
            test_net_endpoint_op_cb, NULL, NULL);

        struct test_net_endpoint_op_data * op_data = test_net_tl_op_data(op);
        op_data->m_ep_id = net_endpoint_id(endpoint);
        op_data->m_action = *action;
    }
}

