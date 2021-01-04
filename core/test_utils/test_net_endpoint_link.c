#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "test_memory.h"
#include "test_net_endpoint_link.h"

test_net_endpoint_link_t
test_net_endpoint_link_create(test_net_endpoint_t a, test_net_endpoint_t z) {
    test_net_endpoint_link_t link = mem_alloc(test_allocrator(), sizeof(struct test_net_endpoint_link));

    test_net_endpoint_write_policy_clear(a);
    test_net_endpoint_write_policy_clear(z);

    a->m_write_policy.m_type = test_net_endpoint_write_link;
    a->m_write_policy.m_link.m_link = link;
    link->m_a = a;

    z->m_write_policy.m_type = test_net_endpoint_write_link;
    z->m_write_policy.m_link.m_link = link;
    link->m_z = z;
    
    return link;
}

void test_net_endpoint_link_free(test_net_endpoint_link_t link) {
    assert(link->m_a->m_write_policy.m_type == test_net_endpoint_write_link);
    assert(link->m_a->m_write_policy.m_link.m_link == link);
    assert(link->m_z->m_write_policy.m_type == test_net_endpoint_write_link);
    assert(link->m_z->m_write_policy.m_link.m_link == link);

    bzero(&link->m_a->m_write_policy, sizeof(link->m_a->m_write_policy));
    link->m_a->m_write_policy.m_type = test_net_endpoint_write_mock;
    link->m_a = NULL;

    bzero(&link->m_z->m_write_policy, sizeof(link->m_z->m_write_policy));
    link->m_z->m_write_policy.m_type = test_net_endpoint_write_mock;
    link->m_z = NULL;

    mem_free(test_allocrator(), link);
}

test_net_endpoint_t
test_net_endpoint_link_other(test_net_endpoint_link_t link, test_net_endpoint_t ep) {
    if (link->m_a == ep) {
        return link->m_z;
    }
    else {
        assert(link->m_z == ep);
        return link->m_a;
    }
}
