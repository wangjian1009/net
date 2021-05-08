#include <assert.h>
#include "cmocka_all.h"
#include "net_endpoint.h"
#include "test_net_protocol_endpoint.h"
#include "test_net_protocol_endpoint_link.h"
#include "test_net_tl_op.h"

extern int test_net_protocol_endpoint_input(net_endpoint_t endpoint);

int test_net_protocol_endpoint_init(net_endpoint_t base_endpoint) {
    test_net_protocol_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);

    if (net_endpoint_protocol_debug(base_endpoint) < 2) {
        net_endpoint_set_protocol_debug(base_endpoint, 2);
    }
    
    endpoint->m_input_policy.m_type = test_net_protocol_endpoint_input_mock;
    endpoint->m_state_change_policy = test_net_protocol_endpoint_state_change_noop;

    return 0;
}

void test_net_protocol_endpoint_fini(net_endpoint_t base_endpoint) {
    test_net_protocol_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    test_net_protocol_endpoint_input_policy_clear(endpoint);
}

int test_net_protocol_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t i_from_state) {
    test_net_protocol_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);

    switch(endpoint->m_state_change_policy) {
    case test_net_protocol_endpoint_state_change_noop:
        return 0;
    case test_net_protocol_endpoint_state_change_mock:
        break;
    }

    uint32_t id = net_endpoint_id(base_endpoint);
    const char * state = net_endpoint_state_str(net_endpoint_state(base_endpoint));
    const char * from_state = net_endpoint_state_str(i_from_state);

    check_expected(id);
    check_expected(state);
    check_expected(from_state);
    
    return mock_type(int);
}

net_protocol_t test_net_protocol_create(net_schedule_t schedule, const char * name) {
    return net_protocol_create(
        schedule, name,
        /*protocol*/
        0, NULL, NULL,
        /*endpoint*/
        sizeof(struct test_net_protocol_endpoint),
        test_net_protocol_endpoint_init,
        test_net_protocol_endpoint_fini,
        test_net_protocol_endpoint_input,
        test_net_protocol_endpoint_on_state_change,
        NULL,
        NULL);
}

void test_net_protocol_endpoint_expect_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t state, int rv) {
    test_net_protocol_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    endpoint->m_state_change_policy = test_net_protocol_endpoint_state_change_mock;
    
    expect_value(test_net_protocol_endpoint_on_state_change, id, net_endpoint_id(base_endpoint));
    expect_string(test_net_protocol_endpoint_on_state_change, state, net_endpoint_state_str(state));
    expect_any(test_net_protocol_endpoint_on_state_change, from_state);
    will_return(test_net_protocol_endpoint_on_state_change, rv);
}
