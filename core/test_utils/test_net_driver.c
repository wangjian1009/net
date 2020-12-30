#include "test_net_driver.h"
#include "test_net_timer.h"
#include "test_net_endpoint.h"
#include "test_net_dgram.h"
#include "test_net_acceptor.h"
#include "test_net_watcher.h"

static int test_net_driver_init(net_driver_t driver);
static void test_net_driver_fini(net_driver_t driver);

test_net_driver_t
test_net_driver_create(net_schedule_t schedule, error_monitor_t em) {
    net_driver_t base_driver = net_driver_create(
        schedule,
        "test",
        /*driver*/
        sizeof(struct test_net_driver),
        test_net_driver_init,
        test_net_driver_fini,
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
        test_net_endpoint_close,
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
    return 0;
}

static void test_net_driver_fini(net_driver_t base_driver) {
}

