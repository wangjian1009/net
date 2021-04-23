#include "cmocka_all.h"
#include "cpe/utils/random.h"
#include "test_net_endpoint.h"
#include "net_ssl_tests.h"
#include "net_ssl_stream_driver.h"
#include "net_ssl_stream_endpoint.h"
#include "net_ssl_stream_pair_testenv.h"

static int setup(void **state) {
    net_ssl_stream_pair_testenv_t env = net_ssl_stream_pair_testenv_create();
    *state = env;
    assert_true(
        net_ssl_stream_driver_svr_prepaired(env->m_env->m_stream_driver) == 0);
    return 0;
}

static int teardown(void **state) {
    net_ssl_stream_pair_testenv_t env = *state;
    net_ssl_stream_pair_testenv_free(env);
    return 0;
}

static void net_ssl_stream_pair_big_response(void **state) {
    net_ssl_stream_pair_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream = net_ssl_testenv_create_stream_cli_endpoint(env->m_env, NULL);
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);
    net_endpoint_set_remote_address(cli_stream_base, net_acceptor_address(env->m_acceptor));

    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);
    
    test_net_endpoint_expect_connect_to_acceptor(cli_ssl_base, "1.2.3.4:5678", 0, 0);
    
    assert_true(net_endpoint_connect(cli_stream_base) == 0);

    test_net_driver_run(env->m_env->m_tdriver, 0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    net_ssl_stream_endpoint_t svr_stream = net_ssl_stream_pair_testenv_get_svr_ep(env, cli_stream);
    assert_true(svr_stream != NULL);

    net_endpoint_t svr_stream_base = net_ssl_stream_endpoint_base_endpoint(svr_stream);

    uint32_t response_size = 10 * 1024;
    char * response_data = mem_alloc(test_allocrator(), response_size);
    assert_true(response_data);
    uint32_t i;
    for(i = 0; i< response_size; i++) {
        response_data[i] = 'a' + cpe_rand_dft(27);
    }
    
    /*client -> server*/
    assert_true(net_endpoint_buf_append(cli_stream_base, net_ep_buf_write, response_data, response_size) == 0);
    test_net_driver_run(env->m_env->m_tdriver, 0);

    test_net_endpoint_assert_buf_memory(svr_stream_base, net_ep_buf_read, response_data, response_size);

    /*server -> client*/
    assert_true(net_endpoint_buf_append(svr_stream_base, net_ep_buf_write, response_data, response_size) == 0);
    test_net_driver_run(env->m_env->m_tdriver, 0);

    test_net_endpoint_assert_buf_memory(cli_stream_base, net_ep_buf_read, response_data, response_size);
}

int net_ssl_stream_pair_big_response_tests() {
	const struct CMUnitTest ssl_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ssl_stream_pair_big_response, setup, teardown),
	};
	return cmocka_run_group_tests(ssl_basic_tests, NULL, NULL);
}
