#include "prometheus_testenv.h"

prometheus_testenv_t
prometheus_testenv_create() {
    prometheus_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct prometheus_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    return env;
}

void prometheus_testenv_free(prometheus_testenv_t env) {
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}
