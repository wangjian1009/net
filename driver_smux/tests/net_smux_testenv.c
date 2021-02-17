#include "cpe/pal/pal_string.h"
#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_smux_protocol.h"
#include "net_smux_testenv.h"

net_smux_testenv_t
net_smux_testenv_create() {
    net_smux_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_smux_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);
    env->m_smux_protocol = net_smux_protocol_create(env->m_schedule, "test", test_allocrator(), env->m_em);

    return env;
}

void net_smux_testenv_free(net_smux_testenv_t env) {
    net_smux_protocol_free(env->m_smux_protocol);
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}
