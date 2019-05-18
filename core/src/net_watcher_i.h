#ifndef NET_WATCHER_I_H_INCLEDED
#define NET_WATCHER_I_H_INCLEDED
#include "net_watcher.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_watcher {
    net_driver_t m_driver;
    TAILQ_ENTRY(net_watcher) m_next_for_driver;
    int m_fd;
    void * m_ctx;
    uint8_t m_expect_read;
    uint8_t m_expect_write;
    uint8_t m_in_processing;
    uint8_t m_deleting;
    net_watcher_action_fun_t m_action;
};

void net_watcher_real_free(net_watcher_t watcher);

NET_END_DECL

#endif
