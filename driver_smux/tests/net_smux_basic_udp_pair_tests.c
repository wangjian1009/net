#include "cmocka_all.h"
#include "net_smux_tests.h"
#include "net_smux_testenv.h"
#include "net_smux_testenv_receiver.h"

static int setup(void **state) {
    net_smux_testenv_t env = net_smux_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_smux_testenv_t env = *state;
    net_smux_testenv_free(env);
    return 0;
}

static void net_smux_basic_udp_pair_basic(void **state) {
    net_smux_testenv_t env = *state;

    net_smux_dgram_t svr_dgram = net_smux_testenv_create_dgram_svr(env, "1.2.3.4:5678");
    
    net_smux_dgram_t cli_dgram = net_smux_testenv_create_dgram_cli(env, "2.3.4.5:6789");

    net_smux_session_t cli_session =
        net_smux_testenv_dgram_open_session(env, cli_dgram, "1.2.3.4:5678");
    assert_true(cli_session);

    /*建立连接 */
    test_net_dgram_expect_write_send(net_smux_dgram_dgram(cli_dgram), 0);
    net_smux_stream_t cli_stream = net_smux_session_open_stream(cli_session);
    assert_true(cli_stream);
    uint32_t sid = net_smux_stream_id(cli_stream);

    //test_net_dgram_expect_write_send(net_smux_dgram_dgram(svr_dgram), 0);
    test_net_driver_run(env->m_tdriver, 0);

    net_smux_session_t svr_session = net_smux_testenv_dgram_find_session(env, svr_dgram, "2.3.4.5:6789");
    assert_true(svr_session != NULL);

    net_smux_stream_t svr_stream = net_smux_session_find_stream(svr_session, sid);
    assert_true(svr_stream);

    /*发送数据 */
    net_smux_testenv_receiver_t cli_receiver = net_smux_testenv_receiver_create(env, cli_stream);
    net_smux_testenv_receiver_t svr_receiver = net_smux_testenv_receiver_create(env, svr_stream);

    /*发送数据c->s */
    assert_true(net_smux_stream_write(cli_stream, "abcd", 4) == 4);
    test_net_driver_run(env->m_tdriver, 0);
    
    assert_int_equal(mem_buffer_size(&svr_receiver->m_buffer), 4);
    assert_memory_equal(mem_buffer_make_continuous(&svr_receiver->m_buffer, 0), "abcd", 4);
}

int net_smux_basic_udp_pair_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(net_smux_basic_udp_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
