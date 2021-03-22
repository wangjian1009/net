#include "prometheus_manager.h"
#include "prometheus_process_testenv.h"

prometheus_process_testenv_t
prometheus_process_testenv_create() {
    prometheus_process_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct prometheus_process_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_vfs_env = vfs_testenv_create(env->m_em);
    env->m_manager = prometheus_manager_create(test_allocrator(), env->m_em);
    env->m_provider =
        prometheus_process_provider_create(
            env->m_manager, env->m_em, test_allocrator(), NULL, NULL);
    return env;
}

void prometheus_process_testenv_free(prometheus_process_testenv_t env) {
    prometheus_process_provider_free(env->m_provider);
    prometheus_manager_free(env->m_manager);
    vfs_testenv_free(env->m_vfs_env);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}
