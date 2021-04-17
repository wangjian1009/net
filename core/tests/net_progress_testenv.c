#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_progress_testenv.h"

net_progress_testenv_t
net_progress_testenv_create() {
    net_progress_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_progress_testenv));
    env->m_env = net_core_testenv_create();
    mem_buffer_init(&env->m_buffer, test_allocrator());
    return env;
}

void net_progress_testenv_free(net_progress_testenv_t env) {
    mem_buffer_clear(&env->m_buffer);
    net_core_testenv_free(env->m_env);
    mem_free(test_allocrator(), env);
}

void net_progress_testenv_data_watch(void * ctx, net_progress_t progress) {
    net_progress_testenv_t env = ctx;

    switch(net_progress_runing_mode(progress)) {
    case net_progress_runing_mode_read: {
        uint32_t size;
        void * data;
        while((data = net_progress_buf_peak(progress, &size))) {
            mem_buffer_append(&env->m_buffer, data, size);
            net_progress_buf_consume(progress, size);
        }
        break;
    }
    case net_progress_runing_mode_write:
        break;
    }
}

net_progress_t
net_progress_testenv_run_cmd_read(net_progress_testenv_t env, const char * cmd, const char * argv[]) {
    net_progress_t progress = 
        net_progress_auto_create(
            env->m_env->m_schedule, cmd, argv, net_progress_runing_mode_read,
            net_progress_testenv_data_watch, env, net_progress_debug_text);

    return progress;
}

const char * net_progress_testenv_buffer_to_string(net_progress_testenv_t env) {
    char * buf = mem_buffer_make_continuous(&env->m_buffer, 1);
    buf[mem_buffer_size(&env->m_buffer)] = 0;
    return buf;
}
