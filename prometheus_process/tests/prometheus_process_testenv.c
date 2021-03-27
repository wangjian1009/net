#include "cmocka_all.h"
#include "prometheus_manager.h"
#include "prometheus_process_testenv.h"

prometheus_process_testenv_t
prometheus_process_testenv_create() {
    prometheus_process_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct prometheus_process_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_vfs_env = test_vfs_testenv_create(env->m_em);
    env->m_manager = prometheus_manager_create(test_allocrator(), env->m_em);
    env->m_provider =
        prometheus_process_provider_create(
            env->m_manager,
            env->m_em, test_allocrator(),
            env->m_vfs_env->m_mgr,
            "/test/limits",
            "/test/proc");

    mem_buffer_init(&env->m_dump_buffer, test_allocrator());
    return env;
}

void prometheus_process_testenv_free(prometheus_process_testenv_t env) {
    mem_buffer_clear(&env->m_dump_buffer);
    prometheus_process_provider_free(env->m_provider);
    prometheus_manager_free(env->m_manager);
    test_vfs_testenv_free(env->m_vfs_env);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

const char * prometheus_process_testenv_collect(prometheus_process_testenv_t env) {
    return prometheus_manager_collect_dump(&env->m_dump_buffer, env->m_manager);
}

void prometheus_process_install_limits(prometheus_process_testenv_t env, const char * data) {
    assert_true(
        test_vfs_testenv_install_file_str(env->m_vfs_env, "/test/limits", data) == 0);
}
