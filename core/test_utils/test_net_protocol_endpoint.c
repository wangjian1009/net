#include <assert.h>
#include "cmocka_all.h"
#include "net_endpoint.h"
#include "test_net_protocol_endpoint.h"
#include "test_net_protocol_endpoint_link.h"
#include "test_net_tl_op.h"

extern int test_net_protocol_endpoint_input(net_endpoint_t endpoint);

int test_net_protocol_endpoint_init(net_endpoint_t base_endpoint) {
    test_net_protocol_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);

    endpoint->m_input_policy.m_type = test_net_protocol_endpoint_input_mock;
    
    return 0;
}

void test_net_protocol_endpoint_fini(net_endpoint_t base_endpoint) {
    test_net_protocol_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    test_net_protocol_endpoint_input_policy_clear(endpoint);
}

int test_net_protocol_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state) {
    return 0;
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
        NULL);
}
