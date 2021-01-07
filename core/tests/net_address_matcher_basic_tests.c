#include "cmocka_all.h"
#include "net_address_matcher.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "test_memory.h"

struct net_address_matcher_test_ctx {
    net_schedule_t m_schedule;
};

static int setup(void **state) {
    struct net_address_matcher_test_ctx * ctx =
        mem_alloc(test_allocrator(), sizeof(struct net_address_matcher_test_ctx));
    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    struct net_address_matcher_test_ctx * ctx = *state;
    mem_free(test_allocrator(), ctx);
    return 0;
}

static void basic(void **state) {
    struct net_address_matcher_test_ctx * ctx = *state;
}

int net_core_address_matcher_basic_tests() {
	const struct CMUnitTest address_matcher_basic_tests[] = {
		cmocka_unit_test_setup_teardown(basic, setup, teardown),
	};
	return cmocka_run_group_tests(address_matcher_basic_tests, NULL, NULL);
}
