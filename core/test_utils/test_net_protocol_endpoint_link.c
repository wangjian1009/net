#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "test_memory.h"
#include "test_net_protocol_endpoint_link.h"

test_net_protocol_endpoint_link_t
test_net_protocol_endpoint_link_create(test_net_protocol_endpoint_t a, test_net_protocol_endpoint_t z, int64_t input_delay_ms) {
    test_net_protocol_endpoint_link_t link = mem_alloc(test_allocrator(), sizeof(struct test_net_protocol_endpoint_link));

    test_net_protocol_endpoint_input_policy_clear(a);
    test_net_protocol_endpoint_input_policy_clear(z);

    a->m_input_policy.m_type = test_net_protocol_endpoint_input_link;
    a->m_input_policy.m_link.m_link = link;
    a->m_input_policy.m_link.m_input_delay_ms = input_delay_ms;
    link->m_a = a;

    z->m_input_policy.m_type = test_net_protocol_endpoint_input_link;
    z->m_input_policy.m_link.m_link = link;
    z->m_input_policy.m_link.m_input_delay_ms = input_delay_ms;
    link->m_z = z;
    
    return link;
}

void test_net_protocol_endpoint_link_free(test_net_protocol_endpoint_link_t link) {
    assert(link->m_a->m_input_policy.m_type == test_net_protocol_endpoint_input_link);
    assert(link->m_a->m_input_policy.m_link.m_link == link);
    assert(link->m_z->m_input_policy.m_type == test_net_protocol_endpoint_input_link);
    assert(link->m_z->m_input_policy.m_link.m_link == link);

    bzero(&link->m_a->m_input_policy, sizeof(link->m_a->m_input_policy));
    link->m_a->m_input_policy.m_type = test_net_protocol_endpoint_input_mock;
    link->m_a = NULL;

    bzero(&link->m_z->m_input_policy, sizeof(link->m_z->m_input_policy));
    link->m_z->m_input_policy.m_type = test_net_protocol_endpoint_input_mock;
    link->m_z = NULL;

    mem_free(test_allocrator(), link);
}

test_net_protocol_endpoint_t
test_net_protocol_endpoint_link_other(test_net_protocol_endpoint_link_t link, test_net_protocol_endpoint_t ep) {
    if (link->m_a == ep) {
        return link->m_z;
    }
    else {
        assert(link->m_z == ep);
        return link->m_a;
    }
}

