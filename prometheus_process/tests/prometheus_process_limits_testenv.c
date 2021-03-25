#include "cmocka_all.h"
#include "cpe/utils/string_utils.h"
#include "prometheus_process_limits_testenv.h"

prometheus_process_limits_testenv_t
prometheus_process_limits_testenv_create() {
    prometheus_process_limits_testenv_t env
        = mem_alloc(test_allocrator(), sizeof(struct prometheus_process_limits_testenv));
    env->m_env = prometheus_process_testenv_create();
    env->m_row_count = 0;
    return env;
}

void prometheus_process_limits_testenv_free(prometheus_process_limits_testenv_t env) {
    uint32_t i;
    for(i = 0; i < env->m_row_count; i++) {
        prometheus_process_limits_row_fini(env->m_env->m_provider, &env->m_rows[i]);
    }
    env->m_row_count = 0;
    
    prometheus_process_testenv_free(env->m_env);
    mem_free(test_allocrator(), env);
}

void prometheus_process_limits_testenv_on_row(void * ctx, prometheus_process_limits_row_t row) {
    prometheus_process_limits_testenv_t env = ctx;

    assert_true(env->m_row_count < CPE_ARRAY_SIZE(env->m_rows));

    prometheus_process_limits_row_t save_row = &env->m_rows[env->m_row_count];
    prometheus_process_limits_row_init(env->m_env->m_provider, save_row);

    save_row->soft = row->soft;
    save_row->hard = row->hard;
    prometheus_process_limits_row_set_units(env->m_env->m_provider, save_row, row->units);
    prometheus_process_limits_row_set_limit(env->m_env->m_provider, save_row, row->limit);
}

int prometheus_process_limits_testenv_load(prometheus_process_limits_testenv_t env) {
    return prometheus_process_limits_load(env->m_env->m_provider, env, prometheus_process_limits_testenv_on_row);
}

prometheus_process_limits_row_t
prometheus_process_limits_testenv_find_row(
    prometheus_process_limits_testenv_t env, const char * limit)
{
    uint32_t i;
    for(i = 0; i < env->m_row_count; i++) {
        prometheus_process_limits_row_t row = &env->m_rows[i];
        if (cpe_str_cmp_opt(row->limit, limit) == 0) return row;
    }

    return NULL;
}


