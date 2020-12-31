#ifndef TEST_UTILS_NET_TIMER_H_INCLEDED
#define TEST_UTILS_NET_TIMER_H_INCLEDED
#include "net_timer.h"
#include "test_net_driver.h"

struct test_net_timer {
    test_net_driver_t m_driver;
    TAILQ_ENTRY(test_net_timer) m_next;
    test_net_tl_op_t m_tl_op;
};

int test_net_timer_init(net_timer_t base_timer);
void test_net_timer_fini(net_timer_t base_timer);
void test_net_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds);
void test_net_timer_cancel(net_timer_t base_timer);
uint8_t test_net_timer_is_active(net_timer_t base_timer);

#endif
