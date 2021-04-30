#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_ndt7_testenv.h"

net_ndt7_testenv_t
net_ndt7_testenv_create() {
    net_ndt7_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ndt7_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);
    return env;
}

void net_ndt7_testenv_free(net_ndt7_testenv_t env) {
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}
