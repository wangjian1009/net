#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_ws_testenv.h"
#include "net_ws_cli_protocol.h"
#include "net_ws_cli_stream_driver.h"
#include "net_ws_cli_endpoint_i.h"
#include "net_ws_svr_endpoint_i.h"

net_ws_testenv_t net_ws_testenv_create() {
    net_ws_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ws_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_test_protocol = test_net_protocol_create(env->m_schedule, "test-protocol");
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);

    env->m_cli_protocol = net_ws_cli_protocol_create(env->m_schedule, NULL, test_allocrator(), env->m_em);
    
    env->m_cli_stream_driver = net_ws_cli_stream_driver_create(
        env->m_schedule, NULL,
        env->m_cli_protocol, net_driver_from_data(env->m_tdriver),
        test_allocrator(), env->m_em);

    env->m_svr_driver = net_ws_svr_driver_create(
        env->m_schedule, NULL, net_driver_from_data(env->m_tdriver),
        test_allocrator(), env->m_em);
    
    return env;
}

void net_ws_testenv_free(net_ws_testenv_t env) {
    test_net_driver_free(env->m_tdriver);
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

net_endpoint_t net_ws_testenv_cli_stream_ep_create(net_ws_testenv_t env) {
    net_endpoint_t endpoint =
        net_endpoint_create(
            net_driver_from_data(env->m_cli_stream_driver),
            env->m_test_protocol,
            NULL);

    net_endpoint_set_driver_debug(endpoint, 1);

    return endpoint;
}

net_acceptor_t
net_ws_testenv_create_svr_acceptor(
    net_ws_testenv_t env, const char * str_address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx)
{
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_acceptor_t acceptor =
        net_acceptor_create(
            net_driver_from_data(env->m_svr_driver),
            env->m_test_protocol,
            address, 0,
            on_new_endpoint, on_new_endpoint_ctx);

    net_address_free(address);

    return acceptor;
}

net_endpoint_t net_ws_testenv_create_svr_endpoint(net_ws_testenv_t env) {
    net_endpoint_t endpoint =
        net_ws_svr_endpoint_create(env->m_svr_driver, env->m_test_protocol);

    net_endpoint_set_driver_debug(endpoint, 1);

    return endpoint;
}
