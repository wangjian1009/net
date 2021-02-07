#include "cmocka_all.h"
#include "net_dns_entry.h"
#include "net_dns_testenv.h"
#include "net_dns_tests.h"

static int setup(void **state) {
    net_dns_testenv_t env = net_dns_testenv_create();
    *state = env;

    net_dns_testenv_setup_records(
        env,
        "addr1:\n"
        "  - sub.addr1\n"
        "  - 127.0.0.1\n"
        "sub.addr1: 127.0.0.2\n"
        "sub.addr2: 127.0.0.2\n"
        );

    return 0;
}

static int teardown(void **state) {
    net_dns_testenv_t env = *state;
    net_dns_testenv_free(env);
    return 0;
}

    
void test_records_basic(void **state) {
    net_dns_testenv_t env = *state;
    
    assert_true(net_dns_testenv_find_entry(env, "addr1") != NULL);
    assert_true(net_dns_testenv_find_entry(env, "sub.addr1") != NULL);
    
    assert_string_equal(net_dns_testenv_hostname_by_ip(env, "127.0.0.1"), "addr1");
    assert_string_equal(net_dns_testenv_hostname_by_ip(env, "127.0.0.2"), "sub.addr2");
    
    assert_string_equal(net_dns_testenv_hostnames_by_ip(env, "127.0.0.1"), "addr1");
    assert_string_equal(net_dns_testenv_hostnames_by_ip(env, "127.0.0.2"), "sub.addr2,sub.addr1");
}

void test_records_add_circle_l1(void **state) {
}

int net_dns_records_case_basic(void) {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(test_records_basic, setup, teardown),
		cmocka_unit_test_setup_teardown(test_records_add_circle_l1, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
