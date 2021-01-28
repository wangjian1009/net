#include "cmocka_all.h"
#include "net_address_matcher.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "test_memory.h"
#include "test_error.h"

struct net_address_matcher_test_ctx {
    test_error_monitor_t m_tem;
    net_schedule_t m_schedule;
    net_address_matcher_t m_address_matcher;
};

static int setup(void **state) {
    struct net_address_matcher_test_ctx * ctx =
        mem_alloc(test_allocrator(), sizeof(struct net_address_matcher_test_ctx));
    *state = ctx;
    ctx->m_tem = test_error_monitor_create();
    ctx->m_schedule = net_schedule_create(test_allocrator(), test_error_monitor_em(ctx->m_tem));
    ctx->m_address_matcher = net_address_matcher_create(ctx->m_schedule);
    return 0;
}

static int teardown(void **state) {
    struct net_address_matcher_test_ctx * ctx = *state;

    net_address_matcher_free(ctx->m_address_matcher);
    net_schedule_free(ctx->m_schedule);
    test_error_monitor_free(ctx->m_tem);
    
    mem_free(test_allocrator(), ctx);
    return 0;
}

static void rule_basic(void **state) {
    struct net_address_matcher_test_ctx * ctx = *state;
    assert_true(
        net_address_matcher_add_rule(ctx->m_address_matcher, "\\.*google\\.*") != NULL);

    assert_true(
        net_address_rule_lookup(ctx->m_address_matcher, "www.google.com") != NULL);
}

int net_address_matcher_basic_tests() {
	const struct CMUnitTest address_matcher_basic_tests[] = {
		cmocka_unit_test_setup_teardown(rule_basic, setup, teardown),
	};
	return cmocka_run_group_tests(address_matcher_basic_tests, NULL, NULL);
}
