#ifndef NET_LOG_ENV_I_H_INCLEDED
#define NET_LOG_ENV_I_H_INCLEDED
#include "net_log_env.h"
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_env {
    net_log_schedule_t m_schedule;
    TAILQ_ENTRY(net_log_env) m_next;
    const char * m_url;
};

net_log_env_t net_log_env_create(net_log_schedule_t schedule, const char * url);
void net_log_env_free(net_log_env_t env);

NET_END_DECL

#endif
