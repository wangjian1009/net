#ifndef NET_WATCHER_H_INCLEDED
#define NET_WATCHER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

struct net_watcher_it {
    net_watcher_t (*next)(net_watcher_it_t it);
    char data[64];
};

net_watcher_t net_watcher_create(net_driver_t driver, int fd);
void net_watcher_free(net_watcher_t watcher);


/*net_watcher_it*/
#define net_watcher_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
