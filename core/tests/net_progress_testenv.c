#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_progress_testenv.h"

net_progress_testenv_t
net_progress_testenv_create() {
    net_progress_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_progress_testenv));
    env->m_env = net_core_testenv_create();
    return env;
}

void net_progress_testenv_free(net_progress_testenv_t env) {
    net_core_testenv_free(env->m_env);
    mem_free(test_allocrator(), env);
}

void net_progress_testenv_data_watch(void * ctx, net_progress_t progress) {
}

net_progress_t
net_progress_testenv_run_cmd_read(net_progress_testenv_t env, const char * cmd) {
    net_progress_t progress = 
        net_progress_auto_create(
            env->m_env->m_schedule, cmd, net_progress_runing_mode_read,
            net_progress_testenv_data_watch, env);

    return progress;
}
