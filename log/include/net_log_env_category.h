#ifndef NET_LOG_ENV_CATEGORY_H_INCLEDED
#define NET_LOG_ENV_CATEGORY_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

struct net_log_env_category_it {
    net_log_env_category_t (*next)(net_log_env_category_it_t it);
    char data[64];
};

#define net_log_env_category_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
