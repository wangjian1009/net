#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "net_schedule.h"
#include "test_net_tl_op.h"
#include "test_net_endpoint_action.h"
#include "test_net_endpoint.h"
#include "test_net_endpoint_link.h"

void test_net_endpoint_apply_action_i(test_net_driver_t tdriver, net_endpoint_t base_endpoint, test_net_endpoint_action_t action) {
    switch(action->m_type) {
    case test_net_endpoint_action_noop:
        break;
    case test_net_endpoint_action_buf_copy:
        assert_return_code(
            net_endpoint_buf_append_from_self(
                base_endpoint, action->m_buf_copy.m_buf_type,
                net_ep_buf_write, 0),
            0);
        break;
    case test_net_endpoint_action_buf_erase:
        net_endpoint_buf_consume(
            base_endpoint, net_ep_buf_write,
            net_endpoint_buf_size(base_endpoint, net_ep_buf_write));
        break;
    case test_net_endpoint_action_buf_link: {
        test_net_endpoint_t endpoint = test_net_endpoint_cast(base_endpoint);
        assert_true(endpoint);
        assert_true(endpoint->m_link);
        test_net_endpoint_t other = test_net_endpoint_link_other(endpoint->m_link, endpoint);
        net_endpoint_t base_other_endpoint = net_endpoint_from_data(other);

        struct test_net_endpoint_action other_action;
        other_action.m_type = test_net_endpoint_action_buf_write;

        other_action.m_buf_write.m_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
        if (other_action.m_buf_write.m_size > 0) {
            other_action.m_buf_write.m_data = mem_buffer_alloc(&tdriver->m_setup_buffer, other_action.m_buf_write.m_size);
            if (net_endpoint_buf_recv(
                    base_endpoint, net_ep_buf_write,
                    other_action.m_buf_write.m_data, &other_action.m_buf_write.m_size) == 0)
            {
                test_net_endpoint_apply_action_delay(tdriver, base_other_endpoint, &other_action, action->m_buf_link.m_delay_ms);
            }
        }
        break;
    }
    case test_net_endpoint_action_buf_write:
        if (net_endpoint_driver_debug(base_endpoint) >= 2) {
            CPE_INFO(
                tdriver->m_em, "test: %s: << %d data",
                net_endpoint_dump(net_schedule_tmp_buffer(net_endpoint_schedule(base_endpoint)), base_endpoint),
                action->m_buf_write.m_size);
        }
        net_endpoint_buf_append(base_endpoint, net_ep_buf_read, action->m_buf_write.m_data, action->m_buf_write.m_size);
        break;
    case test_net_endpoint_action_disable:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        }
        break;
    case test_net_endpoint_action_error:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        }
        break;
    case test_net_endpoint_action_delete:
        net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        break;
    }
}

struct test_net_endpoint_op_data {
    test_net_driver_t m_tdriver;
    uint32_t m_ep_id;
    struct test_net_endpoint_action m_action;
};

static void test_net_endpoint_op_cb(void * ctx, test_net_tl_op_t op) {
    struct test_net_endpoint_op_data * op_data = test_net_tl_op_data(op);

    assert_true(op_data->m_ep_id != 0);

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(op->m_driver));
    net_endpoint_t endpoint = net_endpoint_find(schedule, op_data->m_ep_id);
    if (endpoint == NULL || net_endpoint_state(endpoint) == net_endpoint_state_deleting) return;

    test_net_endpoint_apply_action_i(op_data->m_tdriver, endpoint, &op_data->m_action);
}

void test_net_endpoint_apply_action_delay(
    test_net_driver_t tdriver, net_endpoint_t endpoint,
    test_net_endpoint_action_t action, int64_t delay_ms)
{
    test_net_tl_op_t op = test_net_tl_op_create(
        tdriver, delay_ms,
        sizeof(struct test_net_endpoint_op_data),
        test_net_endpoint_op_cb, NULL, NULL);

    struct test_net_endpoint_op_data * op_data = test_net_tl_op_data(op);
    op_data->m_tdriver = tdriver;
    op_data->m_ep_id = net_endpoint_id(endpoint);
    op_data->m_action = *action;
}

void test_net_endpoint_apply_action(
    test_net_driver_t tdriver, net_endpoint_t endpoint,
    test_net_endpoint_action_t action, int64_t delay_ms)
{
    if (delay_ms == 0) {
        test_net_endpoint_apply_action_i(tdriver, endpoint, action);
    }
    else {
        test_net_endpoint_apply_action_delay(tdriver, endpoint, action, delay_ms);
    }
}

