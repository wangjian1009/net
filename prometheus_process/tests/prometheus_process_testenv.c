#include "cpe/pal/pal_unistd.h"
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
            env->m_vfs_env->m_mgr);

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

void prometheus_process_install_linux_limits(prometheus_process_testenv_t env, const char * data) {
    int pid = (int)getpid();
    char limit_path[64];
    snprintf(limit_path, sizeof(limit_path), "/proc/%d/limits", pid);

    assert_true(
        test_vfs_testenv_install_file_str(env->m_vfs_env, limit_path, data) == 0);
}

void prometheus_process_install_linux_stat(prometheus_process_testenv_t env, const char * data) {
    int pid = (int)getpid();
    char stat_path[64];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    assert_true(
        test_vfs_testenv_install_file_str(env->m_vfs_env, stat_path, data) == 0);
}

void prometheus_process_install_linux_fds(prometheus_process_testenv_t env, uint32_t count) {
    int pid = (int)getpid();
    char path[64];
    size_t n = snprintf(path, sizeof(path), "/proc/%d/fd", pid);

    assert_true(test_vfs_testenv_install_dir(env->m_vfs_env, path) == 0);

    snprintf(path + n, sizeof(path) - n, "/.");
    assert_true(test_vfs_testenv_install_dir(env->m_vfs_env, path) == 0);
    
    snprintf(path + n, sizeof(path) - n, "/..");
    assert_true(test_vfs_testenv_install_dir(env->m_vfs_env, path) == 0);

    uint32_t i;
    for(i = 0; i < count; ++i) {
        snprintf(path + n, sizeof(path) - n, "/%d", i + 1);
        assert_true(test_vfs_testenv_install_file_str(env->m_vfs_env, path, "") == 0);
    }
}
