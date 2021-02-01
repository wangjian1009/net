#include "cmocka_all.h"
#include "cpe/utils/time_utils.h"
#include "test_memory.h"
#include "test_net_driver.h"
#include "test_net_timer.h"
#include "test_net_endpoint.h"
#include "test_net_dgram.h"
#include "test_net_acceptor.h"
#include "test_net_watcher.h"
#include "test_net_tl_op.h"

static int test_net_driver_init(net_driver_t driver);
static void test_net_driver_fini(net_driver_t driver);
static int64_t test_net_driver_time(net_driver_t driver);

test_net_driver_t
test_net_driver_create(net_schedule_t schedule, error_monitor_t em) {
    net_driver_t base_driver = net_driver_create(
        schedule,
        "test",
        /*driver*/
        sizeof(struct test_net_driver),
        test_net_driver_init,
        test_net_driver_fini,
        test_net_driver_time,
        /*timer*/
        sizeof(struct test_net_timer),
        test_net_timer_init,
        test_net_timer_fini,
        test_net_timer_active,
        test_net_timer_cancel,
        test_net_timer_is_active,
        /*acceptor*/
        sizeof(struct test_net_acceptor),
        test_net_acceptor_init,
        test_net_acceptor_fini,
        /*endpoint*/
        sizeof(struct test_net_endpoint),
        test_net_endpoint_init,
        test_net_endpoint_fini,
        test_net_endpoint_connect,
        test_net_endpoint_update,
        test_net_endpoint_set_no_delay,
        test_net_endpoint_get_mss,
        /*dgram*/
        sizeof(struct test_net_dgram),
        test_net_dgram_init,
        test_net_dgram_fini,
        test_net_dgram_send,
        /*watcher*/
        sizeof(struct test_net_watcher),
        test_net_watcher_init,
        test_net_watcher_fini,
        test_net_watcher_update);

    test_net_driver_t driver = net_driver_data(base_driver);
    driver->m_em = em;
    
    return driver;
}

void test_net_driver_free(test_net_driver_t driver) {
    net_driver_free(net_driver_from_data(driver));
}

static int test_net_driver_init(net_driver_t base_driver) {
    test_net_driver_t driver = net_driver_data(base_driver);
    driver->m_em = NULL;
    TAILQ_INIT(&driver->m_timers);
    TAILQ_INIT(&driver->m_acceptors);
    TAILQ_INIT(&driver->m_endpoints);
    TAILQ_INIT(&driver->m_dgrams);
    TAILQ_INIT(&driver->m_watchers);

    driver->m_cur_time_ms = cur_time_ms();
    TAILQ_INIT(&driver->m_tl_ops);

    mem_buffer_init(&driver->m_setup_buffer, test_allocrator());

    return 0;
}

static void test_net_driver_fini(net_driver_t base_driver) {
    test_net_driver_t driver = net_driver_data(base_driver);

    assert_true(TAILQ_EMPTY(&driver->m_timers));
    assert_true(TAILQ_EMPTY(&driver->m_acceptors));
    assert_true(TAILQ_EMPTY(&driver->m_endpoints));
    assert_true(TAILQ_EMPTY(&driver->m_dgrams));
    assert_true(TAILQ_EMPTY(&driver->m_watchers));

    while(!TAILQ_EMPTY(&driver->m_tl_ops)) {
        test_net_tl_op_free(TAILQ_FIRST(&driver->m_tl_ops));
    }
    
    mem_buffer_clear(&driver->m_setup_buffer);
}

static int64_t test_net_driver_time(net_driver_t base_driver) {
    test_net_driver_t driver = net_driver_data(base_driver);
    return driver->m_cur_time_ms;
}

test_net_driver_t
test_net_driver_cast(net_driver_t driver) {
    return net_driver_init_fun(driver) == test_net_driver_init ? net_driver_data(driver) : NULL;
}
