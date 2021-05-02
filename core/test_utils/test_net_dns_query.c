#include "cmocka_all.h"
#include "net_address.h"
#include "test_memory.h"
#include "test_net_dns_query.h"
#include "net_dns_query.h"
#include "test_net_tl_op.h"

enum test_net_dns_query_mock_action_type {
    test_net_dns_query_mock_from_cache,
    test_net_dns_query_mock_once,
};

struct test_net_dns_query_mock_action {
    enum test_net_dns_query_mock_action_type m_type;
    char * m_responses;
};

struct test_net_dns_query_mock_setup {
    enum test_net_dns_query_lifecycle m_lifecycle;
    struct test_net_dns_query_mock_action m_action;
    int64_t m_delay_ms;
};

void test_net_dns_query_apply_action(net_dns_query_t base_query, struct test_net_dns_query_mock_action * action) {
}

struct test_net_dns_query_op {
    struct test_net_dns_query_mock_action m_action;
};

void test_net_dns_query_op_cb(void * ctx, test_net_tl_op_t op) {
    net_dns_query_t base_query = ctx;
    struct test_net_dns_query_op * query_op = test_net_tl_op_data(op);
    test_net_dns_query_apply_action(base_query, &query_op->m_action);
}

int test_net_dns_query_init(
    void * ctx, net_dns_query_t base_query, net_address_t i_hostname, net_dns_query_type_t i_query_type, const char * policy)
{
    test_net_dns_t tdns = ctx;
    struct test_net_dns_query * query = net_dns_query_data(base_query);
    const char * query_type = net_dns_query_type_str(i_query_type);
    char hostname_buf[256];
    const char * hostname = net_address_host_inline(hostname_buf, sizeof(hostname_buf), i_hostname);

    check_expected(hostname);
    check_expected(query_type);
    check_expected(policy);

    struct test_net_dns_query_mock_setup * setup = mock_type(struct test_net_dns_query_mock_setup *);
    query->m_lifecycle = setup->m_lifecycle;

    if (setup->m_delay_ms == 0) {
        test_net_dns_query_apply_action(base_query, &setup->m_action);
    }
    else {
        test_net_tl_op_t op = test_net_tl_op_create(
            tdns->m_tdriver, setup->m_delay_ms,
            sizeof(struct test_net_dns_query_op),
            test_net_dns_query_op_cb, base_query, NULL);
        assert_true(op != NULL);

        struct test_net_dns_query_op * query_op = test_net_tl_op_data(op);
        query_op->m_action = setup->m_action;
    }

    return 0;
}

void test_net_dns_query_fini(void * ctx, net_dns_query_t base_query) {
    struct test_net_dns_query * query = net_dns_query_data(base_query);

    switch(query->m_lifecycle) {
    case test_net_dns_query_lifecycle_mock:
        break;
    case test_net_dns_query_lifecycle_noop:
        break;
    }
}

void test_net_dns_expect_query_cache(test_net_dns_t dns, const char * i_hostname, int64_t delay_ms) {
    const char * hostname = mem_buffer_strdup(&dns->m_tdriver->m_setup_buffer, i_hostname);
    const char * query_type = net_dns_query_type_str(net_dns_query_ipv4);

    expect_string(test_net_dns_query_init, hostname, hostname);
    expect_string(test_net_dns_query_init, query_type, query_type);
    expect_any(test_net_dns_query_init, policy);

    struct test_net_dns_query_mock_setup * setup =
        mem_buffer_alloc(&dns->m_tdriver->m_setup_buffer, sizeof(struct test_net_dns_query_mock_setup));
    setup->m_lifecycle = test_net_dns_query_lifecycle_noop;
    setup->m_action.m_type = test_net_dns_query_mock_from_cache;
    setup->m_action.m_responses = NULL;
    setup->m_delay_ms = delay_ms;
    
    will_return(test_net_dns_query_init, setup);
}

void test_net_dns_expect_query_once(test_net_dns_t dns, const char * i_hostname, const char * responses, int64_t delay_ms) {
    const char * hostname = mem_buffer_strdup(&dns->m_tdriver->m_setup_buffer, i_hostname);
    const char * query_type = net_dns_query_type_str(net_dns_query_ipv4);

    expect_string(test_net_dns_query_init, hostname, hostname);
    expect_string(test_net_dns_query_init, query_type, query_type);
    expect_any(test_net_dns_query_init, policy);

    struct test_net_dns_query_mock_setup * setup =
        mem_buffer_alloc(&dns->m_tdriver->m_setup_buffer, sizeof(struct test_net_dns_query_mock_setup));
    setup->m_lifecycle = test_net_dns_query_lifecycle_noop;
    setup->m_action.m_type = test_net_dns_query_mock_once;
    setup->m_action.m_responses = mem_buffer_strdup(&dns->m_tdriver->m_setup_buffer, responses);
    setup->m_delay_ms = delay_ms;
    
    will_return(test_net_dns_query_init, setup);
}
