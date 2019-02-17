#ifndef NET_WATCHER_I_H_INCLEDED
#define NET_WATCHER_I_H_INCLEDED
#include "net_watcher.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_watcher {
    net_driver_t m_driver;
    TAILQ_ENTRY(net_watcher) m_next_for_driver;
    struct cpe_hash_entry m_hh;
    int m_fd;
    void * m_ctx;
    uint8_t m_in_processing;
    uint8_t m_deleting;
    uint8_t m_expect_read;
    uint8_t m_expect_write;
    net_watcher_action_fun_t m_action;
};

void net_watcher_real_free(net_watcher_t watcher);

uint32_t net_watcher_hash(net_watcher_t o, void * user_data);
int net_watcher_eq(net_watcher_t l, net_watcher_t r, void * user_data);

NET_END_DECL

#endif
