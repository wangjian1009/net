#ifndef TEST_UTILS_NET_DRIVER_H_INCLEDED
#define TEST_UTILS_NET_DRIVER_H_INCLEDED
#include "cpe/utils/error.h"
#include "net_driver.h"

typedef struct test_net_driver * test_net_driver_t;
typedef struct test_net_timer * test_net_timer_t;
typedef struct test_net_acceptor * test_net_acceptor_t;
typedef struct test_net_endpoint * test_net_endpoint_t;
typedef struct test_net_dgram * test_net_dgram_t;
typedef struct test_net_watcher * test_net_watcher_t;

struct test_net_driver {
    error_monitor_t m_em;
};

test_net_driver_t test_net_driver_create(net_schedule_t schedule, error_monitor_t em);
void test_net_driver_free(test_net_driver_t driver);

#endif
