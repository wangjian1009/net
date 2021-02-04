#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_ssl_testenv.h"
#include "net_ssl_endpoint.h"
#include "net_ssl_stream_endpoint.h"

net_ssl_testenv_t net_ssl_testenv_create() {
    net_ssl_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ssl_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_test_protocol = test_net_protocol_create(env->m_schedule, "test-protocol");
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);

    env->m_stream_driver = net_ssl_stream_driver_create(
        env->m_schedule, NULL, net_driver_from_data(env->m_tdriver),
        test_allocrator(), env->m_em);

    return env;
}

void net_ssl_testenv_free(net_ssl_testenv_t env) {
    test_net_driver_free(env->m_tdriver);
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

net_acceptor_t
net_ssl_testenv_create_stream_acceptor(
    net_ssl_testenv_t env, const char * str_address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx)
{
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_acceptor_t acceptor =
        net_acceptor_create(
            net_driver_from_data(env->m_stream_driver),
            env->m_test_protocol,
            address, 0,
            on_new_endpoint, on_new_endpoint_ctx);

    net_address_free(address);

    return acceptor;
}

net_ssl_stream_endpoint_t
net_ssl_testenv_create_stream_endpoint(net_ssl_testenv_t env) {
    net_ssl_stream_endpoint_t endpoint =
        net_ssl_stream_endpoint_create(env->m_stream_driver, env->m_test_protocol);
    assert_true(endpoint);

    net_endpoint_t base_endpoint = net_ssl_stream_endpoint_base_endpoint(endpoint);

    net_endpoint_set_driver_debug(base_endpoint, 2);

    return endpoint;
}

net_ssl_stream_endpoint_t
net_ssl_testenv_create_stream_cli_endpoint(net_ssl_testenv_t env) {
    net_ssl_stream_endpoint_t endpoint = net_ssl_testenv_create_stream_endpoint(env);
    assert_true(
        net_ssl_stream_endpoint_set_runing_mode(endpoint, net_ssl_endpoint_runing_mode_cli) == 0);
    return endpoint;
}

net_ssl_stream_endpoint_t
net_ssl_testenv_create_stream_svr_endpoint(net_ssl_testenv_t env) {
    net_ssl_stream_endpoint_t endpoint = net_ssl_testenv_create_stream_endpoint(env);
    assert_true(
        net_ssl_stream_endpoint_set_runing_mode(endpoint, net_ssl_endpoint_runing_mode_svr) == 0);
    return endpoint;
}
