#ifndef TEST_UTILS_NET_DRIVER_H_INCLEDED
#define TEST_UTILS_NET_DRIVER_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_driver.h"

typedef struct test_net_driver * test_net_driver_t;
typedef struct test_net_timer * test_net_timer_t;
typedef struct test_net_acceptor * test_net_acceptor_t;
typedef struct test_net_endpoint * test_net_endpoint_t;
typedef struct test_net_endpoint_link * test_net_endpoint_link_t;
typedef struct test_net_dgram * test_net_dgram_t;
typedef struct test_net_watcher * test_net_watcher_t;
typedef struct test_net_tl_op * test_net_tl_op_t;

typedef TAILQ_HEAD(test_net_timer_list, test_net_timer) test_net_timer_list_t;
typedef TAILQ_HEAD(test_net_acceptor_list, test_net_acceptor) test_net_acceptor_list_t;
typedef TAILQ_HEAD(test_net_endpoint_list, test_net_endpoint) test_net_endpoint_list_t;
typedef TAILQ_HEAD(test_net_dgram_list, test_net_dgram) test_net_dgram_list_t;
typedef TAILQ_HEAD(test_net_watcher_list, test_net_watcher) test_net_watcher_list_t;
typedef TAILQ_HEAD(test_net_tl_op_list, test_net_tl_op) test_net_tl_op_list_t;

struct test_net_driver {
    error_monitor_t m_em;

    test_net_timer_list_t m_timers;
    test_net_acceptor_list_t m_acceptors;
    test_net_endpoint_list_t m_endpoints;
    test_net_dgram_list_t m_dgrams;
    test_net_watcher_list_t m_watchers;

    int64_t m_cur_time_ms;
    test_net_tl_op_list_t m_tl_ops;

    struct mem_buffer m_setup_buffer;
};

test_net_driver_t test_net_driver_create(net_schedule_t schedule, error_monitor_t em);
void test_net_driver_free(test_net_driver_t driver);
int64_t test_net_driver_run(test_net_driver_t driver, int64_t duration_ms);

test_net_driver_t test_net_driver_cast(net_driver_t driver);

net_endpoint_t
test_net_driver_find_endpoint_by_remote_addr(test_net_driver_t driver, net_address_t address);

net_acceptor_t
test_net_driver_find_acceptor_by_addr(test_net_driver_t driver, net_address_t address);

#endif
