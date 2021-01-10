#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/time_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_sock_driver_i.h"
#include "net_sock_acceptor_i.h"
#include "net_sock_endpoint_i.h"
#include "net_sock_dgram.h"

static void net_sock_driver_fini(net_driver_t driver);
static int64_t net_sock_driver_time(net_driver_t driver);

net_sock_driver_t
net_sock_driver_create(
    net_schedule_t schedule,
    const char * driver_name,
    /*driver*/
    uint16_t driver_capacity,
    net_sock_driver_init_fun_t driver_init,
    net_sock_driver_fini_fun_t driver_fini,
    /*timer*/
    uint16_t timer_capacity,
    net_timer_init_fun_t timer_init,
    net_timer_fini_fun_t timer_fini,
    net_timer_schedule_fun_t timer_schedule,
    net_timer_cancel_fun_t timer_cancel,
    net_timer_is_active_fun_t timer_is_active,
    /*watcher*/
    uint16_t watcher_capacity,
    net_watcher_init_fun_t watcher_init,
    net_watcher_fini_fun_t watcher_fini,
    net_watcher_update_fun_t watcher_update)
{
    net_driver_t base_driver;

    base_driver = net_driver_create(
        schedule,
        driver_name,
        /*driver*/
        sizeof(struct net_sock_driver) + driver_capacity,
        net_sock_driver_init,
        net_sock_driver_fini,
        net_sock_driver_time,
        /*timer*/
        timer_capacity,
        timer_init,
        timer_fini,
        timer_schedule,
        timer_cancel,
        timer_is_active,
        /*acceptor*/
        sizeof(struct net_sock_acceptor),
        net_sock_acceptor_init,
        net_sock_acceptor_fini,
        /*endpoint*/
        sizeof(struct net_sock_endpoint),
        net_sock_endpoint_init,
        net_sock_endpoint_fini,
        net_sock_endpoint_connect,
        net_sock_endpoint_close,
        net_sock_endpoint_update,
        net_sock_endpoint_set_no_delay,
        net_sock_endpoint_get_mss,
        /*dgram*/
        sizeof(struct net_sock_dgram),
        net_sock_dgram_init,
        net_sock_dgram_fini,
        net_sock_dgram_send,
        /*watcher*/
        watcher_capacity,
        watcher_init,
        watcher_fini,
        watcher_update);

    if (base_driver == NULL) return NULL;

    net_sock_driver_t driver = net_driver_data(base_driver);
    if (driver_init) {
        if (driver_init(driver) != 0) {
            net_sock_driver_free(driver);
            return NULL;
        }
    }
    
    driver->m_init = driver_init;
    driver->m_fini = driver_fini;
    
    return driver;
}

int net_sock_driver_init(net_driver_t base_driver) {
    net_sock_driver_t driver = net_driver_data(base_driver);
    net_schedule_t schedule = net_driver_schedule(base_driver);

    driver->m_alloc = net_schedule_allocrator(schedule);
    driver->m_em = net_schedule_em(schedule);
    driver->m_init = NULL;
    driver->m_fini = NULL;

    return 0;
}

static void net_sock_driver_fini(net_driver_t base_driver) {
    net_sock_driver_t driver = net_driver_data(base_driver);
    if (driver->m_fini) {
        driver->m_fini(driver);
    }

    driver->m_init = NULL;
    driver->m_fini = NULL;
}

static int64_t net_sock_driver_time(net_driver_t driver) {
    return cur_time_ms();
}

void net_sock_driver_free(net_sock_driver_t driver) {
    net_driver_free(net_driver_from_data(driver));
}

void * net_sock_driver_data(net_sock_driver_t sock_driver) {
    return sock_driver + 1;
}

void * net_sock_driver_data_from_base_driver(net_driver_t base_driver) {
    return net_sock_driver_data(net_driver_data(base_driver));
}

net_sock_driver_t net_sock_driver_from_data(void * data) {
    return ((net_sock_driver_t)data) - 1;
}

net_driver_t net_sock_driver_base_driver(net_sock_driver_t sock_driver) {
    return net_driver_from_data(sock_driver);
}

net_schedule_t net_sock_driver_schedule(net_sock_driver_t driver) {
    return net_driver_schedule(net_driver_from_data(driver));
}

mem_buffer_t net_sock_driver_tmp_buffer(net_sock_driver_t driver) {
    return net_schedule_tmp_buffer(net_sock_driver_schedule(driver));
}
