#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "test_memory.h"
#include "test_net_endpoint_link.h"

test_net_endpoint_link_t
test_net_endpoint_link_create(test_net_endpoint_t a, test_net_endpoint_t z, int64_t write_delay_ms) {
    test_net_endpoint_link_t link = mem_alloc(test_allocrator(), sizeof(struct test_net_endpoint_link));

    if (a->m_link) {
        test_net_endpoint_link_free(a->m_link);
        assert(a->m_link == NULL);
    }

    if (z->m_link) {
        test_net_endpoint_link_free(z->m_link);
        assert(z->m_link == NULL);
    }
    
    a->m_link = link;
    link->m_a = a;

    a->m_write_policy.m_type = test_net_endpoint_write_action;
    a->m_write_policy.m_action.m_action.m_type = test_net_endpoint_action_buf_link;
    a->m_write_policy.m_action.m_action.m_buf_link.m_delay_ms = write_delay_ms;
    a->m_write_policy.m_action.m_delay_ms = 0;

    z->m_link = link;
    link->m_z = z;
    
    z->m_write_policy.m_type = test_net_endpoint_write_action;
    z->m_write_policy.m_action.m_action.m_type = test_net_endpoint_action_buf_link;
    z->m_write_policy.m_action.m_action.m_buf_link.m_delay_ms = write_delay_ms;
    z->m_write_policy.m_action.m_delay_ms = 0;
    
    return link;
}

void test_net_endpoint_link_free(test_net_endpoint_link_t link) {
    assert(link->m_a->m_link == link);
    assert(link->m_z->m_link == link);

    link->m_a->m_write_policy.m_type = test_net_endpoint_write_mock;
    link->m_a->m_link = NULL;
    link->m_a = NULL;

    link->m_z->m_write_policy.m_type = test_net_endpoint_write_mock;
    link->m_z->m_link = NULL;
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

