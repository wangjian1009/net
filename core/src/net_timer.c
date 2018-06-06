#include "net_timer_i.h"
#include "net_driver_i.h"

net_timer_t net_timer_create(
    net_driver_t driver,
    net_timer_process_fun_t process_fun, void * process_ctx)
{
    net_schedule_t schedule = driver->m_schedule;
    net_timer_t timer;

    timer = TAILQ_FIRST(&driver->m_free_timers);
    if (timer) {
        TAILQ_REMOVE(&driver->m_free_timers, timer, m_next_for_driver);
    }
    else {
        timer = mem_alloc(schedule->m_alloc, sizeof(struct net_timer) + driver->m_timer_capacity);
        if (timer == NULL) {
            CPE_ERROR(schedule->m_em, "net_timer_create: alloc fail");
            return NULL;
        }
    }

    timer->m_driver = driver;
    timer->m_process_fun = process_fun;
    timer->m_process_ctx = process_ctx;

    if (driver->m_timer_init(timer) != 0) {
        CPE_ERROR(schedule->m_em, "net_timer_create: init fail");
        TAILQ_INSERT_TAIL(&driver->m_free_timers, timer, m_next_for_driver);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&driver->m_timers, timer, m_next_for_driver);
    
    return timer;
}

net_timer_t net_timer_auto_create(
    net_schedule_t schedule,
    net_timer_process_fun_t process_fun, void * process_ctx)
{
    net_driver_t driver;

    TAILQ_FOREACH(driver, &schedule->m_drivers, m_next_for_schedule) {
        if (driver->m_timer_schedule) {
            return net_timer_create(driver, process_fun, process_ctx);
        }
    }
    
    CPE_ERROR(schedule->m_em, "net_timer_auto_create: no driver support timer");
    return NULL;
}

void net_timer_free(net_timer_t timer) {
    net_driver_t driver = timer->m_driver;

    driver->m_timer_fini(timer);

    TAILQ_REMOVE(&driver->m_timers, timer, m_next_for_driver);
    TAILQ_INSERT_TAIL(&driver->m_free_timers, timer, m_next_for_driver);
}

void net_timer_real_free(net_timer_t timer) {
    net_driver_t driver = timer->m_driver;
    
    TAILQ_REMOVE(&driver->m_free_timers, timer, m_next_for_driver);

    mem_free(driver->m_schedule->m_alloc, timer);
}

net_schedule_t net_timer_schedule(net_timer_t timer) {
    return timer->m_driver->m_schedule;
}

net_driver_t net_timer_driver(net_timer_t timer) {
    return timer->m_driver;
}

void * net_timer_data(net_timer_t timer) {
    return timer + 1;
}

net_timer_t net_timer_from_data(void * data) {
    return ((net_timer_t)data) - 1;
}

uint8_t net_timer_is_active(net_timer_t timer) {
    return timer->m_driver->m_timer_is_active(timer);
}

void net_timer_active(net_timer_t timer, int32_t delay_ms) {
    timer->m_driver->m_timer_schedule(timer, delay_ms);
}

void net_timer_cancel(net_timer_t timer) {
    timer->m_driver->m_timer_cancel(timer);
}

void net_timer_process(net_timer_t timer) {
    if (timer->m_process_fun) {
        timer->m_process_fun(timer, timer->m_process_ctx);
    }
}

