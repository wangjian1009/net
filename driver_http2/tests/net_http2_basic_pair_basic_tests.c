#include "cmocka_all.h"
#include "net_http2_protocol.h"
#include "net_http2_tests.h"
#include "net_http2_testenv.h"
#include "net_http2_testenv_receiver.h"

static int setup(void **state) {
    net_http2_testenv_t env = net_http2_testenv_create();
    *state = env;
    net_http2_protocol_set_no_http_messaging(env->m_protocol, 1);
    return 0;
}

static int teardown(void **state) {
    net_http2_testenv_t env = *state;
    net_http2_testenv_free(env);
    return 0;
}

static void net_http2_basic_pair_basic(void **state) {
    net_http2_testenv_t env = *state;

    net_acceptor_t svr_acceptor = net_http2_testenv_create_basic_acceptor(env, "1.2.3.4:5678");
    
    net_http2_endpoint_t cli_ep = net_http2_testenv_create_basic_ep_cli(env, "1.2.3.4:5678");
    net_endpoint_t cli_ep_base = net_http2_endpoint_base_endpoint(cli_ep);

    test_net_endpoint_expect_connect_to_acceptor(cli_ep_base, "1.2.3.4:5678", 100, 0);
    
    assert_true(net_endpoint_connect(cli_ep_base) == 0);

    /*验证状态*/
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_string_equal(
        net_http2_endpoint_state_str(net_http2_endpoint_state(cli_ep)),
        net_http2_endpoint_state_str(net_http2_endpoint_state_init));
    
    /*等待连接成功 */
    test_net_driver_run(env->m_tdriver, 100);

    net_endpoint_t svr_ep_base = test_net_endpoint_linked_other(env->m_tdriver, cli_ep_base);
    assert_true(svr_ep_base != NULL);

    net_http2_endpoint_t svr_ep = net_http2_endpoint_cast(svr_ep_base);
    assert_true(svr_ep);

    /*验证连接成功状态 */
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    assert_string_equal(
        net_http2_endpoint_state_str(net_http2_endpoint_state(cli_ep)),
        net_http2_endpoint_state_str(net_http2_endpoint_state_streaming));

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(svr_ep_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    assert_string_equal(
        net_http2_endpoint_state_str(net_http2_endpoint_state(svr_ep)),
        net_http2_endpoint_state_str(net_http2_endpoint_state_streaming));
    
    /*创建stream连接 */
    net_http2_req_t cli_req = net_http2_req_create(cli_ep);
    assert_true(cli_req);

    assert_true(net_http2_req_add_req_head(cli_req, "a", "av") == 0);
    assert_true(net_http2_req_start_request(cli_req, 1) == 0);

    assert_string_equal(
        net_http2_req_state_str(net_http2_req_state(cli_req)),
        net_http2_req_state_str(net_http2_req_state_head_sended));
    
    test_net_driver_run(env->m_tdriver, 0);
    
    net_http2_stream_t cli_stream = net_http2_req_stream(cli_req);
    assert_true(cli_stream);

    uint32_t stream_id = net_http2_stream_id(cli_stream);

    net_http2_stream_t svr_stream = net_http2_endpoint_find_stream(svr_ep, stream_id);
    assert_true(svr_stream);

    net_http2_req_t svr_req = net_http2_stream_req(svr_stream);
    assert_true(svr_req);

    assert_string_equal(
        net_http2_req_state_str(net_http2_req_state(svr_req)),
        net_http2_req_state_str(net_http2_req_state_head_received));

    assert_string_equal(
        net_http2_req_find_req_header(svr_req, "a"),
        "av");

    /*发送响应头 */
    assert_true(net_http2_req_add_res_head(svr_req, "b", "bv") == 0);
    assert_true(net_http2_req_start_response(svr_req, 1) == 0);

    test_net_driver_run(env->m_tdriver, 0);
    
    assert_string_equal(
        net_http2_req_state_str(net_http2_req_state(svr_req)),
        net_http2_req_state_str(net_http2_req_state_established));
    
    assert_string_equal(
        net_http2_req_state_str(net_http2_req_state(cli_req)),
        net_http2_req_state_str(net_http2_req_state_established));

    assert_string_equal(
        net_http2_req_find_res_header(cli_req, "b"),
        "bv");

    /*发送数据c->s */
    net_http2_testenv_receiver_t cli_receiver = net_http2_testenv_create_req_receiver(env, cli_req);
    net_http2_testenv_receiver_t svr_receiver = net_http2_testenv_create_req_receiver(env, svr_req);

    assert_true(net_http2_req_append(cli_req, "abcd", 4, 1) == 0);
    test_net_driver_run(env->m_tdriver, 0);
    
    assert_int_equal(mem_buffer_size(&svr_receiver->m_buffer), 4);
    assert_memory_equal(mem_buffer_make_continuous(&svr_receiver->m_buffer, 0), "abcd", 4);

    /*发送数据s->c (关闭写) */
    assert_true(net_http2_req_append(svr_req, "efgh", 4, 0) == 0);
    test_net_driver_run(env->m_tdriver, 0);
    
    assert_int_equal(mem_buffer_size(&cli_receiver->m_buffer), 4);
    assert_memory_equal(mem_buffer_make_continuous(&cli_receiver->m_buffer, 0), "efgh", 4);

    assert_string_equal(
        net_http2_req_state_str(net_http2_req_state(cli_req)),
        net_http2_req_state_str(net_http2_req_state_read_closed));

    assert_string_equal(
        net_http2_req_state_str(net_http2_req_state(svr_req)),
        net_http2_req_state_str(net_http2_req_state_write_closed));

    /*关闭最后一端写 */
    assert_true(net_http2_req_append(cli_req, "ef", 2, 0) == 0);
    test_net_driver_run(env->m_tdriver, 0);
    
    assert_int_equal(mem_buffer_size(&svr_receiver->m_buffer), 6);
    assert_memory_equal(mem_buffer_make_continuous(&svr_receiver->m_buffer, 0), "abcdef", 6);

    assert_string_equal(
        net_http2_req_state_str(net_http2_req_state(cli_req)),
        net_http2_req_state_str(net_http2_req_state_closed));

    assert_string_equal(
        net_http2_req_state_str(net_http2_req_state(svr_req)),
        net_http2_req_state_str(net_http2_req_state_closed));

    assert_true(net_http2_endpoint_find_stream(svr_ep, stream_id) == NULL);
    assert_true(net_http2_endpoint_find_stream(cli_ep, stream_id) == NULL);
}

int net_http2_basic_pair_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(net_http2_basic_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
