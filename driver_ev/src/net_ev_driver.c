#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_ev_driver_i.h"
#include "net_ev_acceptor_i.h"
#include "net_ev_endpoint.h"
#include "net_ev_dgram.h"
#include "net_ev_timer.h"

static int net_ev_driver_init(net_driver_t driver);
static void net_ev_driver_fini(net_driver_t driver);

net_ev_driver_t
net_ev_driver_create(net_schedule_t schedule, struct ev_loop * ev_loop) {
    net_driver_t base_driver;

    base_driver = net_driver_create(
        schedule,
        "ev",
        /*driver*/
        sizeof(struct net_ev_driver),
        net_ev_driver_init,
        net_ev_driver_fini,
        /*timer*/
        sizeof(struct net_ev_timer),
        net_ev_timer_init,
        net_ev_timer_fini,
        net_ev_timer_active,
        net_ev_timer_cancel,
        net_ev_timer_is_active,
        /*acceptor*/
        sizeof(struct net_ev_acceptor),
        net_ev_acceptor_init,
        net_ev_acceptor_fini,
        /*endpoint*/
        sizeof(struct net_ev_endpoint),
        net_ev_endpoint_init,
        net_ev_endpoint_fini,
        net_ev_endpoint_connect,
        net_ev_endpoint_close,
        net_ev_endpoint_on_output,
        /*dgram*/
        sizeof(struct net_ev_dgram),
        net_ev_dgram_init,
        net_ev_dgram_fini,
        net_ev_dgram_send);

    if (base_driver == NULL) return NULL;

    net_ev_driver_t driver = net_driver_data(base_driver);
    driver->m_ev_loop = ev_loop;

    return net_driver_data(base_driver);
}

static int net_ev_driver_init(net_driver_t base_driver) {
    net_ev_driver_t driver = net_driver_data(base_driver);
    net_schedule_t schedule = net_driver_schedule(base_driver);

    driver->m_alloc = net_schedule_allocrator(schedule);
    driver->m_em = net_schedule_em(schedule);
    driver->m_ev_loop = NULL;
    driver->m_sock_process_fun = NULL;
    driver->m_sock_process_ctx = NULL;
    driver->m_data_monitor_fun = NULL;
    driver->m_data_monitor_ctx = NULL;

    return 0;
}

static void net_ev_driver_fini(net_driver_t base_driver) {
}

void net_ev_driver_free(net_ev_driver_t driver) {
    net_driver_free(net_driver_from_data(driver));
}

net_ev_driver_t net_ev_driver_cast(net_driver_t driver) {
    return strcmp(net_driver_name(driver), "ev") == 0 ? net_driver_data(driver) : NULL;
}

struct ev_loop * net_ev_driver_loop(net_ev_driver_t driver) {
    return driver->m_ev_loop;
}

void net_ev_driver_set_sock_create_processor(
    net_ev_driver_t driver,
    net_ev_driver_sock_create_process_fun_t process_fun,
    void * process_ctx)
{
    driver->m_sock_process_fun = process_fun;
    driver->m_sock_process_ctx = process_ctx;
}
    
void net_ev_driver_set_data_monitor(
    net_ev_driver_t driver,
    net_data_monitor_fun_t monitor_fun, void * monitor_ctx)
{
    driver->m_data_monitor_fun = monitor_fun;
    driver->m_data_monitor_ctx = monitor_ctx;
}

net_schedule_t net_ev_driver_schedule(net_ev_driver_t driver) {
    return net_driver_schedule(net_driver_from_data(driver));
}

mem_buffer_t net_ev_driver_tmp_buffer(net_ev_driver_t driver) {
    return net_schedule_tmp_buffer(net_ev_driver_schedule(driver));
}
