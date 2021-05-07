#include "cmocka_all.h"
#include "test_net_endpoint.h"
#include "test_ws_endpoint_receiver.h"
#include "net_ws_tests.h"
#include "net_ws_testenv.h"

static int setup(void **state) {
    net_ws_testenv_t env = net_ws_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_testenv_free(env);
    return 0;
}

static void net_ws_pair_basic(void **state) {
    net_ws_testenv_t env = *state;

    net_ws_endpoint_t cli_ep;
    net_ws_endpoint_t svr_ep;
    net_ws_testenv_cli_create_pair_established(env, &cli_ep, &svr_ep, "1.1.2.3:5678", "/a/b");

    test_net_driver_run(env->m_tdriver, 0);

    /*验证svr状态 */
    assert_string_equal(
        net_ws_endpoint_state_str(net_ws_endpoint_state(svr_ep)),
        net_ws_endpoint_state_str(net_ws_endpoint_state_streaming));

    assert_string_equal(net_ws_endpoint_path(svr_ep), "/a/b");
    
    /*验证cli状态 */
    assert_string_equal(
        net_ws_endpoint_state_str(net_ws_endpoint_state(cli_ep)),
        net_ws_endpoint_state_str(net_ws_endpoint_state_streaming));

    test_net_ws_endpoint_install_receivers(cli_ep);
    test_net_ws_endpoint_install_receivers(svr_ep);
    
    struct test_net_ws_endpoint_action action_noop = { test_net_ws_endpoint_op_noop };
    
    /*client -> server*/
    test_net_ws_endpoint_expect_text_msg(env->m_tdriver, svr_ep, "abcd", &action_noop, 0);
    assert_true(net_ws_endpoint_send_msg_text(cli_ep, "abcd") == 0);
    test_net_driver_run(env->m_tdriver, 0);

    test_net_ws_endpoint_expect_bin_msg(env->m_tdriver, svr_ep, "efgh", 4, &action_noop, 0);
    assert_true(net_ws_endpoint_send_msg_bin(cli_ep, "efgh", 4) == 0);
    test_net_driver_run(env->m_tdriver, 0);

    /*server -> client*/
    test_net_ws_endpoint_expect_text_msg(env->m_tdriver, cli_ep, "hijk", &action_noop, 0);
    assert_true(net_ws_endpoint_send_msg_text(svr_ep, "hijk") == 0);
    test_net_driver_run(env->m_tdriver, 0);

    test_net_ws_endpoint_expect_bin_msg(env->m_tdriver, cli_ep, "lmno", 4, &action_noop, 0);
    assert_true(net_ws_endpoint_send_msg_bin(svr_ep, "lmno", 4) == 0);
    test_net_driver_run(env->m_tdriver, 0);
}

static void net_ws_pair_text_msg_disable_ep(void **state) {
    net_ws_testenv_t env = *state;

    net_ws_endpoint_t cli_ep;
    net_ws_endpoint_t svr_ep;
    net_ws_testenv_cli_create_pair_established(env, &cli_ep, &svr_ep, "1.1.2.3:5678", "/a/b");
    test_net_driver_run(env->m_tdriver, 0);

    test_net_ws_endpoint_install_receivers(svr_ep);
    
    /*client -> server*/
    struct test_net_ws_endpoint_action action1 = { test_net_ws_endpoint_op_disable };
    test_net_ws_endpoint_expect_text_msg(env->m_tdriver, svr_ep, "abcd", &action1, 0);
    assert_true(net_ws_endpoint_send_msg_text(cli_ep, "abcd") == 0);
    test_net_driver_run(env->m_tdriver, 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(net_ws_endpoint_base_endpoint(svr_ep))),
        net_endpoint_state_str(net_endpoint_state_disable));

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(net_ws_endpoint_base_endpoint(cli_ep))),
        net_endpoint_state_str(net_endpoint_state_disable));
}

static void net_ws_pair_text_msg_delete_ep(void **state) {
    net_ws_testenv_t env = *state;

    net_ws_endpoint_t cli_ep;
    net_ws_endpoint_t svr_ep;
    net_ws_testenv_cli_create_pair_established(env, &cli_ep, &svr_ep, "1.1.2.3:5678", "/a/b");
    test_net_driver_run(env->m_tdriver, 0);

    uint32_t svr_ep_id = net_endpoint_id(net_ws_endpoint_base_endpoint(svr_ep));

    test_net_ws_endpoint_install_receivers(svr_ep);
    
    /*client -> server*/
    struct test_net_ws_endpoint_action action1 = { test_net_ws_endpoint_op_delete };
    test_net_ws_endpoint_expect_text_msg(env->m_tdriver, svr_ep, "abcd", &action1, 0);
    assert_true(net_ws_endpoint_send_msg_text(cli_ep, "abcd") == 0);
    test_net_driver_run(env->m_tdriver, 0);

    assert_true(net_endpoint_find(env->m_schedule, svr_ep_id) == NULL);
}

CPE_BEGIN_TEST_SUIT(net_ws_pair_basic_tests)
    cmocka_unit_test_setup_teardown(net_ws_pair_basic, setup, teardown),
    cmocka_unit_test_setup_teardown(net_ws_pair_text_msg_disable_ep, setup, teardown),
    cmocka_unit_test_setup_teardown(net_ws_pair_text_msg_delete_ep, setup, teardown),
CPE_END_TEST_SUIT(net_ws_pair_basic_tests)
