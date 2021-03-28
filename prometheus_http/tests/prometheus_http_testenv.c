#include "cpe/pal/pal_unistd.h"
#include "cmocka_all.h"
#include "net_schedule.h"
#include "prometheus_manager.h"
#include "prometheus_http_testenv.h"

prometheus_http_testenv_t
prometheus_http_testenv_create() {
    prometheus_http_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct prometheus_http_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_manager = prometheus_manager_create(test_allocrator(), env->m_em);
    env->m_net_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);
    env->m_net_driver = test_net_driver_create(env->m_net_schedule, env->m_em);
    env->m_http_svr_env = net_http_svr_testenv_create(env->m_net_schedule);
    env->m_http_processor =
        prometheus_http_processor_create(
            env->m_http_svr_env->m_protocol,
            env->m_manager,
            env->m_em, test_allocrator());

    return env;
}

void prometheus_http_testenv_free(prometheus_http_testenv_t env) {
    prometheus_http_processor_free(env->m_http_processor);
    prometheus_manager_free(env->m_manager);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}
