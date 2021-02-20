#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_pair.h"
#include "net_xkcp_testenv.h"
#include "net_xkcp_driver.h"

net_xkcp_testenv_t net_xkcp_testenv_create() {
    net_xkcp_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_xkcp_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_test_protocol = test_net_protocol_create(env->m_schedule, "test-protocol");
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);

    const char * addition_name = "test";
    
    env->m_xkcp_driver =
        net_xkcp_driver_create(
            env->m_schedule, addition_name,
            net_driver_from_data(env->m_tdriver),
            test_allocrator(), env->m_em);
    net_driver_set_debug(net_driver_from_data(env->m_xkcp_driver), 2);
    
    return env;
}

void net_xkcp_testenv_free(net_xkcp_testenv_t env) {
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

void net_xkcp_testenv_cli_create_pair(
    net_xkcp_testenv_t env, net_xkcp_endpoint_t * cli, net_xkcp_endpoint_t * svr,
    const char * str_address)
{
    /* net_endpoint_t ep[2]; */
    /* assert_true( */
    /*     net_pair_endpoint_make( */
    /*         env->m_schedule, */
    /*         net_protocol_from_data(env->m_protocol), */
    /*         net_protocol_from_data(env->m_protocol), */
    /*         ep) == 0); */

    /* *cli = net_xkcp_endpoint_cast(ep[0]); */
    /* *svr = net_xkcp_endpoint_cast(ep[1]); */

    /* if (str_address) { */
    /*     net_address_t address = net_address_create_auto(env->m_schedule, str_address); */
    /*     net_endpoint_set_remote_address(net_xkcp_endpoint_base_endpoint(*cli), address); */
    /*     net_address_free(address); */
    /* } */
}

void net_xkcp_testenv_cli_create_pair_established(
    net_xkcp_testenv_t env, net_xkcp_endpoint_t * cli, net_xkcp_endpoint_t * svr,
    const char * str_address)
{
    net_xkcp_testenv_cli_create_pair(env, cli, svr, str_address);

    assert_true(net_endpoint_connect(net_xkcp_endpoint_base_endpoint(*cli)) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(net_xkcp_endpoint_base_endpoint(*cli))),
        net_endpoint_state_str(net_endpoint_state_established));

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(net_xkcp_endpoint_base_endpoint(*svr))),
        net_endpoint_state_str(net_endpoint_state_established));
}

net_xkcp_endpoint_t
net_xkcp_testenv_create_ep(net_xkcp_testenv_t env) {
    net_endpoint_t base_endpoint =
        net_endpoint_create(
            net_driver_from_data(env->m_xkcp_driver),
            env->m_test_protocol,
            NULL);

    net_xkcp_endpoint_t endpoint = net_xkcp_endpoint_cast(base_endpoint);
    assert_true(endpoint);

    return endpoint;
}

net_xkcp_endpoint_t
net_xkcp_testenv_create_cli_ep(net_xkcp_testenv_t env, const char * host) {
    net_xkcp_endpoint_t endpoint = net_xkcp_testenv_create_ep(env);
    net_endpoint_t base_endpoint = net_xkcp_endpoint_base_endpoint(endpoint);
    assert_true(base_endpoint != NULL);
        
    net_address_t address = net_address_create_auto(env->m_schedule, host);
    assert_true(address != NULL);
    net_endpoint_set_remote_address(base_endpoint, address);
    net_address_free(address);

    return endpoint;
}

net_xkcp_acceptor_t
net_xkcp_testenv_create_acceptor(
    net_xkcp_testenv_t env, const char * str_address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx)
{
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_acceptor_t acceptor =
        net_acceptor_create(
            net_driver_from_data(env->m_xkcp_driver),
            env->m_test_protocol,
            address, 0,
            on_new_endpoint, on_new_endpoint_ctx);

    net_address_free(address);

    return net_acceptor_data(acceptor);
}

/* net_endpoint_t */
/* net_xkcp_testenv_get_svr_stream(net_xkcp_testenv_t env, net_endpoint_t client_base_ep) { */
/*     /\* net_xkcp_endpoint_t cli_ep = net_endpoint_data(client_base_ep); *\/ */
        
/*     /\* return svr_stream; *\/ */
/*     return NULL; */
/* } */
