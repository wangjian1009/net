#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_ndt7_testenv.h"
#include "net_ndt7_manage.h"

net_ndt7_testenv_t
net_ndt7_testenv_create() {
    net_ndt7_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ndt7_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);
    env->m_tdns = test_net_dns_create(env->m_tdriver);
    env->m_external_svr = test_http_svr_testenv_create(env->m_schedule, env->m_tdriver, env->m_em);

    env->m_ndt_manager =
        net_ndt7_manage_create(
            test_allocrator(), env->m_em, env->m_schedule, net_driver_from_data(env->m_tdriver));
    assert_true(env->m_ndt_manager);

    return env;
}

void net_ndt7_testenv_free(net_ndt7_testenv_t env) {
    test_http_svr_testenv_free(env->m_external_svr);
    net_ndt7_manage_free(env->m_ndt_manager);
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}
