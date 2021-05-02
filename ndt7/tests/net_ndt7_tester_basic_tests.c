#include "net_ndt7_testenv.h"
#include "net_ndt7_tester.h"
#include "net_ndt7_tests.h"

static int setup(void **state) {
    net_ndt7_testenv_t env = net_ndt7_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_ndt7_testenv_t env = *state;
    return 0;
}

static void ndt7_tester_basic(void **state) {
    net_ndt7_testenv_t env = *state;

    net_ndt7_tester_t tester =
        net_ndt7_tester_create(env->m_ndt_manager, net_ndt7_test_download_and_upload);

    test_net_dns_expect_query_response(env->m_tdns, "locate.measurementlab.net", "1.1.1.1", 0);
    
    assert_true(net_ndt7_tester_start(tester) == 0);

    test_net_driver_run(env->m_tdriver, 0);

    assert_string_equal(
        net_ndt7_tester_state_str(net_ndt7_tester_state_done),
        net_ndt7_tester_state_str(net_ndt7_tester_state(tester)));
}

CPE_BEGIN_TEST_SUIT(net_nd7_tester_basic_tests)
    cmocka_unit_test_setup_teardown(ndt7_tester_basic, setup, teardown),
CPE_END_TEST_SUIT(net_nd7_tester_basic_tests)
