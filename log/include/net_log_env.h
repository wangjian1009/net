#ifndef NET_LOG_ENV_H_INCLEDED
#define NET_LOG_ENV_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

struct net_log_env_it {
    net_log_env_t (*next)(net_log_env_it_t it);
    char data[64];
};

void net_log_envs(net_log_schedule_t schedule, net_log_env_it_t env_it);

const char * net_log_env_url(net_log_env_t env);
net_log_env_t net_log_env_find(net_log_schedule_t schedule, const char * url);

void net_log_env_categories(net_log_env_t env, net_log_env_category_it_t category_it);

#define net_log_env_it_next(__it) ((__it)->next(__it))


NET_END_DECL

#endif
