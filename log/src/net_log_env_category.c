#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "net_log_env_category_i.h"

net_log_env_category_t
net_log_env_category_create(net_log_env_t env, net_log_category_t log_category) {
    net_log_schedule_t schedule = env->m_schedule;
    ASSERT_THREAD(schedule->m_main_thread);
    
    net_log_env_category_t category = mem_alloc(schedule->m_alloc, sizeof(struct net_log_env_category));
    if (category == NULL) {
        CPE_ERROR(schedule->m_em, "log: statistic: env %s: cagegory %s: alloc fail", env->m_url, log_category->m_name);
        return NULL;
    }

    category->m_env = env;
    category->m_log_category = log_category;
    
    TAILQ_INSERT_TAIL(&env->m_categories, category, m_next_for_env);
    TAILQ_INSERT_TAIL(&log_category->m_statistics, category, m_next_for_log_cagegory);
    
    return category;
}

void net_log_env_category_free(net_log_env_category_t category) {
    net_log_schedule_t schedule = category->m_env->m_schedule;

    TAILQ_REMOVE(&category->m_log_category->m_statistics, category, m_next_for_log_cagegory);
    TAILQ_REMOVE(&category->m_env->m_categories, category, m_next_for_env);
    
    mem_free(schedule->m_alloc, category);
}

net_log_env_category_t
net_log_env_category_check_create(net_log_schedule_t scheudle, net_log_category_t log_category) {
    net_log_env_category_t category;
    
    TAILQ_FOREACH(category, &log_category->m_statistics, m_next_for_log_cagegory) {
    }
    
    return NULL;
}
