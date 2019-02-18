#ifndef NET_WATCHER_H_INCLEDED
#define NET_WATCHER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef void (*net_watcher_action_fun_t)(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
                                         
struct net_watcher_it {
    net_watcher_t (*next)(net_watcher_it_t it);
    char data[64];
};

net_watcher_t net_watcher_create(net_driver_t driver, int fd, void * ctx, net_watcher_action_fun_t action);
void net_watcher_free(net_watcher_t watcher);

net_watcher_t net_watcher_find(net_schedule_t schedule, int fd);

uint8_t net_watcher_expect_read(net_watcher_t watcher);
uint8_t net_watcher_expect_write(net_watcher_t watcher);
void net_watcher_update(net_watcher_t watcher, uint8_t expect_read, uint8_t expect_write);
void net_watcher_notify(net_watcher_t watcher, uint8_t do_read, uint8_t do_write);

void * net_watcher_data(net_watcher_t watcher);
net_watcher_t net_watcher_from_data(void * data);
net_driver_t net_watcher_driver(net_watcher_t watcher);

/*net_watcher_it*/
#define net_watcher_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
