#include <assert.h>
#include "test_memory.h"
#include "test_net_endpoint_link.h"

test_net_endpoint_link_t
test_net_endpoint_link_create(test_net_endpoint_t a, test_net_endpoint_t z) {
    test_net_endpoint_link_t link = mem_alloc(test_allocrator(), sizeof(struct test_net_endpoint_link));

    assert(a->m_link == NULL);
    assert(z->m_link == NULL);

    a->m_link = link;
    link->m_a = a;

    z->m_link = link;
    link->m_z = z;
    
    return link;
}

void test_net_endpoint_link_free(test_net_endpoint_link_t link) {
    assert(link->m_a->m_link == link);
    assert(link->m_z->m_link == link);

    link->m_a->m_link = NULL;
    link->m_a = NULL;

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

