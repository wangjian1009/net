#include "cmocka_all.h"
#include "net_timer.h"
#include "test_net_timer.h"
#include "test_net_tl_op.h"

int test_net_timer_init(net_timer_t base_timer) {
    test_net_driver_t driver = net_driver_data(net_timer_driver(base_timer));
    test_net_timer_t timer = net_timer_data(base_timer);
    TAILQ_INSERT_TAIL(&driver->m_timers, timer, m_next);
    timer->m_tl_op = NULL;
    return 0;
}

void test_net_timer_fini(net_timer_t base_timer) {
    test_net_driver_t driver = net_driver_data(net_timer_driver(base_timer));
    test_net_timer_t timer = net_timer_data(base_timer);

    if (timer->m_tl_op) {
        test_net_tl_op_free(timer->m_tl_op);
        timer->m_tl_op = NULL;
    }
    
    TAILQ_REMOVE(&driver->m_timers, timer, m_next);
}

void test_net_timer_tl_op_cb(void * ctx, test_net_tl_op_t op) {
    net_timer_t base_timer = ctx;
    test_net_timer_t timer = net_timer_data(base_timer);

    assert_ptr_equal(timer->m_tl_op,  op);
    timer->m_tl_op = NULL;
    
    net_timer_process(base_timer);
}

void test_net_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds) {
    test_net_driver_t driver = net_driver_data(net_timer_driver(base_timer));
    test_net_timer_t timer = net_timer_data(base_timer);
    
    if (timer->m_tl_op) {
        test_net_tl_op_free(timer->m_tl_op);
        timer->m_tl_op = NULL;
    }

    timer->m_tl_op =
        test_net_tl_op_create(
            driver, (int64_t)(delay_milliseconds/1000U), 0,
            test_net_timer_tl_op_cb, base_timer, NULL);
}

void test_net_timer_cancel(net_timer_t base_timer) {
    test_net_timer_t timer = net_timer_data(base_timer);
    
    if (timer->m_tl_op) {
        test_net_tl_op_free(timer->m_tl_op);
        timer->m_tl_op = NULL;
    }
}

uint8_t test_net_timer_is_active(net_timer_t base_timer) {
    test_net_timer_t timer = net_timer_data(base_timer);
    return timer->m_tl_op == NULL ? 0 : 1;
}
