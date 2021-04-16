#ifndef NET_PROGRESS_I_H_INCLEDED
#define NET_PROGRESS_I_H_INCLEDED
#include "net_progress.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_progress {
    net_driver_t m_driver;
    TAILQ_ENTRY(net_progress) m_next_for_driver;
};

NET_END_DECL

#endif
