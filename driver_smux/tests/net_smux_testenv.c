#include "cpe/pal/pal_string.h"
#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_smux_protocol.h"
#include "net_smux_testenv.h"
#include "net_smux_testenv_receiver.h"

net_smux_testenv_t
net_smux_testenv_create() {
    net_smux_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_smux_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);
    env->m_smux_protocol = net_smux_protocol_create(env->m_schedule, "test", test_allocrator(), env->m_em);
    net_protocol_set_debug(net_protocol_from_data(env->m_smux_protocol), 2);
    net_smux_config_init_default(&env->m_smux_config);

    TAILQ_INIT(&env->m_receivers);
    
    return env;
}

void net_smux_testenv_free(net_smux_testenv_t env) {
    net_smux_protocol_free(env->m_smux_protocol);

    while(!TAILQ_EMPTY(&env->m_receivers)) {
        net_smux_testenv_receiver_free(TAILQ_FIRST(&env->m_receivers));
    }
    
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

net_smux_dgram_t
net_smux_testenv_create_dgram_svr(net_smux_testenv_t env, const char * str_address) {
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_smux_dgram_t dgram =
        net_smux_dgram_create(
            env->m_smux_protocol, net_smux_runing_mode_svr,
            net_driver_from_data(env->m_tdriver), address,
            &env->m_smux_config);
    assert_true(dgram);

    net_address_free(address);

    return dgram;
}

net_smux_dgram_t
net_smux_testenv_create_dgram_cli(net_smux_testenv_t env, const char * str_address) {
    net_address_t address = NULL;

    if (str_address) {
        address = net_address_create_auto(env->m_schedule, str_address);
        assert_true(address != NULL);
    }

    net_smux_dgram_t dgram =
        net_smux_dgram_create(
            env->m_smux_protocol, net_smux_runing_mode_cli,
            net_driver_from_data(env->m_tdriver), address,
            &env->m_smux_config);
    assert_true(dgram);

    if (address) {
        net_address_free(address);
    }

    return dgram;
}    

net_smux_session_t
net_smux_testenv_dgram_open_session(
    net_smux_testenv_t env, net_smux_dgram_t dgram, const char * str_address) {
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);
    
    net_smux_session_t session = net_smux_dgram_open_session(dgram, address);

    net_address_free(address);

    return session;
}

net_smux_session_t
net_smux_testenv_dgram_find_session(
    net_smux_testenv_t env, net_smux_dgram_t dgram, const char * str_address) {
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);
    
    net_smux_session_t session = net_smux_dgram_find_session(dgram, address);

    net_address_free(address);

    return session;
}
