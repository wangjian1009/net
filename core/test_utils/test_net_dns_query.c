#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_dns_query.h"
#include "test_memory.h"
#include "test_net_dns_query.h"
#include "test_net_dns_record.h"
#include "test_net_tl_op.h"

enum test_net_dns_query_mock_action_type {
    test_net_dns_query_response,
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

struct test_net_dns_query_create_address_ctx {
    test_net_dns_record_t m_record;
    net_schedule_t m_schedule;
};
    
void test_net_dns_query_create_address_one(void * i_ctx, const char * value) {
    struct test_net_dns_query_create_address_ctx * ctx = i_ctx;

    net_address_t address = net_address_create_auto(ctx->m_schedule, value);
    assert_true(address);

    assert_true(test_net_dns_record_add_resolved(ctx->m_record, address) == 0);

    net_address_free(address);
}
    
void test_net_dns_query_apply_action_response(test_net_dns_t dns, net_dns_query_t base_query, const char * response) {
    struct test_net_dns_query * query = net_dns_query_data(base_query);

    struct test_net_dns_query_create_address_ctx ctx = {
        test_net_dns_record_create(dns, query->m_hostname),
        test_net_dns_schedule(dns)
    };

    cpe_str_list_for_each(response, ',', test_net_dns_query_create_address_one, &ctx);
    
    struct net_address_it all_address;
    test_net_dns_record_resolveds(ctx.m_record, &all_address);

    net_dns_query_notify_result_and_free(
        base_query,
        ctx.m_record->m_resolved_count > 0 ? ctx.m_record->m_resolveds[0] : NULL,
        &all_address);
}

void test_net_dns_query_apply_action(
    test_net_dns_t dns, net_dns_query_t base_query, struct test_net_dns_query_mock_action * action)
{
    switch(action->m_type) {
    case test_net_dns_query_response:
        test_net_dns_query_apply_action_response(dns, base_query, action->m_responses);
        break;
    }
}

struct test_net_dns_query_op {
    test_net_dns_t m_dns;
    struct test_net_dns_query_mock_action m_action;
};

void test_net_dns_query_op_cb(void * ctx, test_net_tl_op_t op) {
    net_dns_query_t base_query = ctx;
    struct test_net_dns_query_op * query_op = test_net_tl_op_data(op);
    test_net_dns_query_apply_action(query_op->m_dns, base_query, &query_op->m_action);
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

    net_schedule_t schedule = test_net_dns_schedule(tdns);
    
    struct test_net_dns_query_mock_setup * setup = mock_type(struct test_net_dns_query_mock_setup *);
    query->m_lifecycle = setup->m_lifecycle;
    query->m_hostname = net_address_copy(schedule, i_hostname);
    
    if (setup->m_delay_ms == 0) {
        test_net_dns_query_apply_action(tdns, base_query, &setup->m_action);
    }
    else {
        test_net_tl_op_t op = test_net_tl_op_create(
            tdns->m_tdriver, setup->m_delay_ms,
            sizeof(struct test_net_dns_query_op),
            test_net_dns_query_op_cb, base_query, NULL);
        assert_true(op != NULL);

        struct test_net_dns_query_op * query_op = test_net_tl_op_data(op);
        query_op->m_dns= tdns;
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
        if (query->m_hostname) {
            net_address_free(query->m_hostname);
            query->m_hostname = NULL;
        }
        break;
    }
}

void test_net_dns_expect_query_response(test_net_dns_t dns, const char * i_hostname, const char * responses, int64_t delay_ms) {
    const char * hostname = mem_buffer_strdup(&dns->m_tdriver->m_setup_buffer, i_hostname);
    const char * query_type = net_dns_query_type_str(net_dns_query_ipv4);

    expect_string(test_net_dns_query_init, hostname, hostname);
    expect_string(test_net_dns_query_init, query_type, query_type);
    expect_any(test_net_dns_query_init, policy);

    struct test_net_dns_query_mock_setup * setup =
        mem_buffer_alloc(&dns->m_tdriver->m_setup_buffer, sizeof(struct test_net_dns_query_mock_setup));
    setup->m_lifecycle = test_net_dns_query_lifecycle_noop;
    setup->m_action.m_type = test_net_dns_query_response;
    setup->m_action.m_responses = mem_buffer_strdup(&dns->m_tdriver->m_setup_buffer, responses);
    setup->m_delay_ms = delay_ms;
    
    will_return(test_net_dns_query_init, setup);
}
