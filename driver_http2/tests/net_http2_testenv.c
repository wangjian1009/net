#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_pair.h"
#include "net_http2_testenv.h"
#include "net_http2_protocol.h"
#include "net_http2_stream_driver.h"

net_http2_testenv_t net_http2_testenv_create() {
    net_http2_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_http2_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_test_protocol = test_net_protocol_create(env->m_schedule, "test-protocol");
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);

    const char * addition_name = "test";
    
    env->m_protocol =
        net_http2_protocol_create(env->m_schedule, addition_name, test_allocrator(), env->m_em);
    net_protocol_set_debug(net_protocol_from_data(env->m_protocol), 2);

    env->m_stream_driver =
        net_http2_stream_driver_create(
            env->m_schedule, addition_name,
            net_driver_from_data(env->m_tdriver),
            test_allocrator(), env->m_em);
    net_driver_set_debug(net_driver_from_data(env->m_stream_driver), 2);
    
    return env;
}

void net_http2_testenv_free(net_http2_testenv_t env) {
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

void net_http2_testenv_create_pair(
    net_http2_testenv_t env, net_http2_endpoint_t * cli, net_http2_endpoint_t * svr,
    const char * str_address)
{
    net_endpoint_t ep[2];
    assert_true(
        net_pair_endpoint_make(
            env->m_schedule,
            net_protocol_from_data(env->m_protocol),
            net_protocol_from_data(env->m_protocol),
            ep) == 0);

    *cli = net_http2_endpoint_cast(ep[0]);
    *svr = net_http2_endpoint_cast(ep[1]);

    /* net_http2_endpoint_set_runing_mode(*cli, net_http2_endpoint_runing_mode_cli); */
    /* net_http2_endpoint_set_runing_mode(*svr, net_http2_endpoint_runing_mode_svr); */

    if (str_address) {
        net_address_t address = net_address_create_auto(env->m_schedule, str_address);
        net_endpoint_set_remote_address(net_http2_endpoint_base_endpoint(*cli), address);
        net_address_free(address);
    }
}

void net_http2_testenv_create_pair_established(
    net_http2_testenv_t env, net_http2_endpoint_t * cli, net_http2_endpoint_t * svr,
    const char * str_address)
{
    net_http2_testenv_create_pair(env, cli, svr, str_address);

    assert_true(net_endpoint_connect(net_http2_endpoint_base_endpoint(*cli)) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(net_http2_endpoint_base_endpoint(*cli))),
        net_endpoint_state_str(net_endpoint_state_established));

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(net_http2_endpoint_base_endpoint(*svr))),
        net_endpoint_state_str(net_endpoint_state_established));
}

net_http2_endpoint_t
net_http2_testenv_svr_ep_create(net_http2_testenv_t env) {
    net_endpoint_t base_endpoint =
        net_endpoint_create(
            net_driver_from_data(env->m_tdriver),
            net_protocol_from_data(env->m_protocol),
            NULL);

    net_http2_endpoint_t endpoint = net_http2_endpoint_cast(base_endpoint);

    //net_http2_endpoint_set_runing_mode(endpoint, net_http2_endpoint_runing_mode_svr);

    return endpoint;
}

net_http2_stream_endpoint_t
net_http2_testenv_stream_ep_create(net_http2_testenv_t env) {
    return net_http2_stream_endpoint_create(env->m_stream_driver, env->m_test_protocol);
}

net_http2_stream_endpoint_t
net_http2_testenv_stream_cli_ep_create(net_http2_testenv_t env, const char * host) {
    net_http2_stream_endpoint_t endpoint = net_http2_testenv_stream_ep_create(env);
    net_endpoint_t base_endpoint = net_http2_stream_endpoint_base_endpoint(endpoint);
    assert_true(base_endpoint != NULL);
        
    net_address_t address = net_address_create_auto(env->m_schedule, host);
    assert_true(address != NULL);
    net_endpoint_set_remote_address(base_endpoint, address);
    net_address_free(address);

    return endpoint;
}

net_acceptor_t
net_http2_testenv_create_acceptor(
    net_http2_testenv_t env, const char * str_address,
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

net_http2_stream_endpoint_t
net_http2_testenv_create_stream(net_http2_testenv_t env) {
    net_http2_stream_endpoint_t endpoint =
        net_http2_stream_endpoint_create(env->m_stream_driver, env->m_test_protocol);

    return endpoint;
}
