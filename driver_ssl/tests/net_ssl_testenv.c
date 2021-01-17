#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_ssl_testenv.h"
#include "net_ssl_cli_endpoint_i.h"

net_ssl_testenv_t net_ssl_testenv_create() {
    net_ssl_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ssl_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);

    env->m_cli_driver = net_ssl_cli_driver_create(
        env->m_schedule, NULL, net_driver_from_data(env->m_tdriver),
        test_allocrator(), env->m_em);

    env->m_svr_driver = net_ssl_svr_driver_create(
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

net_endpoint_t net_ssl_testenv_cli_ep_create(net_ssl_testenv_t env) {
    net_endpoint_t endpoint =
        net_endpoint_create(
            net_driver_from_data(env->m_cli_driver),
            net_schedule_noop_protocol(env->m_schedule),
            NULL);

    net_endpoint_set_driver_debug(endpoint, 1);

    return endpoint;
}

net_endpoint_t net_ssl_testenv_cli_ep_undline(net_endpoint_t ep) {
    net_ssl_cli_endpoint_t ssl_ep = net_endpoint_data(ep);
    return ssl_ep->m_underline;
}

net_acceptor_t
net_ssl_testenv_create_svr_acceptor(
    net_ssl_testenv_t env, const char * str_address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx)
{
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_acceptor_t acceptor =
        net_acceptor_create(
            net_driver_from_data(env->m_svr_driver),
            net_schedule_noop_protocol(env->m_schedule),
            address, 0,
            on_new_endpoint, on_new_endpoint_ctx);

    net_address_free(address);

    return acceptor;
}
