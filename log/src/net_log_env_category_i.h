#ifndef NET_LOG_ENV_CATEGORY_I_H_INCLEDED
#define NET_LOG_ENV_CATEGORY_I_H_INCLEDED
#include "net_log_env_category.h"
#include "net_log_env_i.h"
#include "net_log_category_i.h"

NET_BEGIN_DECL

struct net_log_env_category {
    net_log_env_t m_env;
    TAILQ_ENTRY(net_log_env_category) m_next_for_env;
    net_log_category_t m_log_category;
    TAILQ_ENTRY(net_log_env_category) m_next_for_log_cagegory;
    uint32_t m_send_count;
    uint32_t m_send_success_count;
    uint32_t m_send_fail_count;
};

net_log_env_category_t net_log_env_category_create(net_log_env_t env, net_log_category_t log_category);
void net_log_env_category_free(net_log_env_category_t category);

net_log_env_category_t net_log_env_category_check_create(net_log_schedule_t scheudle, net_log_category_t log_category);

NET_END_DECL

#endif
