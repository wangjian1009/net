#ifndef NET_ANDROID_DRIVER_I_H_INCLEDED
#define NET_ANDROID_DRIVER_I_H_INCLEDED
#include <jni.h>
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/hash.h"
#include "net_schedule.h"
#include "net_android_driver.h"

typedef struct net_android_watcher * net_android_watcher_t;
typedef struct net_android_timer * net_android_timer_t;

struct net_android_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
};

net_schedule_t net_android_driver_schedule(net_android_driver_t driver);
mem_buffer_t net_android_driver_tmp_buffer(net_android_driver_t driver);

#endif
