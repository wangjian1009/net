#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "test_net_endpoint.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_ssl_stream_driver.h"
#include "net_ssl_stream_endpoint.h"
#include "net_ssl_endpoint.h"
#include "net_http_endpoint.h"
#include "net_https_testenv.h"

net_https_testenv_t
net_https_testenv_create() {
    net_https_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_https_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);

    env->m_http_protocol = net_http_test_protocol_create(env->m_schedule, env->m_em, "test");

    env->m_ssl_driver =
        net_ssl_stream_driver_create(
            env->m_schedule,
            "test",
            net_driver_from_data(env->m_tdriver),
            test_allocrator(), env->m_em);
    net_driver_set_debug(net_driver_from_data(env->m_ssl_driver), 1);
    assert_true(net_ssl_stream_driver_svr_prepaired(env->m_ssl_driver) == 0);

    env->m_https_endpoint = NULL;
    env->m_ssl_cli_endpoint = NULL;
    env->m_ssl_svr_endpoint = NULL;

    mem_buffer_init(&env->m_tmp_buffer, test_allocrator());

    return env;
}

void net_https_testenv_free(net_https_testenv_t env) {
    mem_buffer_clear(&env->m_tmp_buffer);
    net_ssl_stream_driver_free(env->m_ssl_driver);
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

net_http_endpoint_t
net_https_testenv_create_ep(net_https_testenv_t env) {
    assert_true(env->m_https_endpoint == NULL);

    env->m_ssl_svr_endpoint =
        net_ssl_stream_endpoint_create(
            env->m_ssl_driver,
            net_schedule_noop_protocol(env->m_schedule));
        
    assert_true(net_ssl_stream_endpoint_set_runing_mode(env->m_ssl_svr_endpoint, net_ssl_endpoint_runing_mode_svr) == 0);
    net_ssl_endpoint_t svr_ssl_endpoint = net_ssl_stream_endpoint_underline(env->m_ssl_svr_endpoint);
    assert_true(svr_ssl_endpoint);
                
    env->m_https_endpoint =
        net_http_endpoint_create(
            net_driver_from_data(env->m_ssl_driver),
            env->m_http_protocol->m_protocol);

    net_endpoint_t base_endpoint = net_endpoint_from_protocol_data(env->m_schedule, env->m_https_endpoint);

    env->m_ssl_cli_endpoint = net_ssl_stream_endpoint_cast(base_endpoint);
    assert_true(env->m_ssl_cli_endpoint);
    assert_true(net_ssl_stream_endpoint_set_runing_mode(env->m_ssl_cli_endpoint, net_ssl_endpoint_runing_mode_cli) == 0);
    net_ssl_endpoint_t cli_ssl_endpoint = net_ssl_stream_endpoint_underline(env->m_ssl_cli_endpoint);
    assert_true(cli_ssl_endpoint);
    
    net_address_t address = net_address_create_auto(env->m_schedule, "1.1.1.1:1");
    net_endpoint_set_remote_address(base_endpoint, address);
    net_address_free(address);

    test_net_endpoint_expect_connect_to_endpoint(
        net_ssl_endpoint_base_endpoint(cli_ssl_endpoint),
        "1.1.1.1:1",
        net_ssl_endpoint_base_endpoint(svr_ssl_endpoint),
        0, 0);
    
    assert_true(net_endpoint_connect(base_endpoint) == 0);

    return env->m_https_endpoint;
}

int net_https_testenv_send_response(net_https_testenv_t env, const char * data) {
    return net_endpoint_buf_append(
        net_ssl_stream_endpoint_base_endpoint(env->m_ssl_svr_endpoint),
        net_ep_buf_write,
        data, strlen(data));
}

const char * net_https_testenv_dump_svr_received(mem_buffer_t buffer, net_https_testenv_t env) {
    assert_true(env->m_ssl_svr_endpoint);

    mem_buffer_clear_data(buffer);
    
    net_endpoint_t base_endpoint = net_ssl_stream_endpoint_base_endpoint(env->m_ssl_svr_endpoint);

    size_t sz = net_endpoint_buf_size(base_endpoint, net_ep_buf_read);

    if (sz == 0) return "";

    void * data = NULL;
    assert_true(net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_read, sz, &data) == 0);
    assert_true(data);

    char * o_buf = mem_buffer_alloc(buffer, sz + 1);
    memcpy(o_buf, data, sz);
    o_buf[sz] = 0;
    return o_buf;
}
