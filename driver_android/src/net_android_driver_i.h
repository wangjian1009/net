#ifndef NET_ANDROID_DRIVER_I_H_INCLEDED
#define NET_ANDROID_DRIVER_I_H_INCLEDED
#include "ev.h"
#include <android/looper.h>
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/hash.h"
#include "net_schedule.h"
#include "net_android_driver.h"

typedef struct net_android_timer * net_android_timer_t;

struct net_android_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    struct ev_loop * m_ev_loop;
    ALooper* m_looper;
};

net_android_driver_t net_android_driver_from_base_driver(net_driver_t base_driver);

net_schedule_t net_android_driver_schedule(net_android_driver_t driver);
mem_buffer_t net_android_driver_tmp_buffer(net_android_driver_t driver);

#endif
