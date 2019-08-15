#include "cpe/pal/pal_string.h"
#include "net_log_env_i.h"
#include "net_log_env_category_i.h"

net_log_env_t
net_log_env_create(net_log_schedule_t schedule, const char * url) {
    size_t url_len = strlen(url) + 1;
    
    net_log_env_t env = mem_alloc(schedule->m_alloc, sizeof(struct net_log_env) + url_len);
    if (env == NULL) {
        CPE_ERROR(schedule->m_em, "log: statistic: env %s: alloc fail", url);
        return NULL;
    }

    env->m_schedule = schedule;
    env->m_url = (char*)(schedule + 1);
    memcpy((char*)(schedule + 1), url, url_len);
    TAILQ_INIT(&env->m_categories);
    
    TAILQ_INSERT_TAIL(&schedule->m_envs, env, m_next);
    
    return env;
}

void net_log_env_free(net_log_env_t env) {
    net_log_schedule_t schedule = env->m_schedule;

    while(TAILQ_EMPTY(&env->m_categories)) {
        net_log_env_category_free(TAILQ_FIRST(&env->m_categories));
    }

    TAILQ_REMOVE(&schedule->m_envs, env, m_next);
    
    mem_free(schedule->m_alloc, env);
}

net_log_env_t net_log_env_find(net_log_schedule_t schedule, const char * url) {
    return NULL;
}
