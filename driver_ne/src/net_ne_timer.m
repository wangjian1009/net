#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_timer.h"
#include "net_driver.h"
#include "net_ne_timer.h"

static void net_ne_timer_timeout_cb(void *arg);
static void net_ne_timer_cancel_cb(void *arg);

int net_ne_timer_init(net_timer_t base_timer) {
    net_ne_timer_t timer = net_timer_data(base_timer);
    net_ne_driver_t driver = net_driver_data(net_timer_driver(base_timer));

    timer->m_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, driver->m_queue);
    timer->m_is_active = 0;
    dispatch_retain(timer->m_timer);
    
    dispatch_set_context(timer->m_timer, timer);
    dispatch_source_set_event_handler_f(timer->m_timer, net_ne_timer_timeout_cb);
    dispatch_source_set_cancel_handler_f(timer->m_timer, net_ne_timer_cancel_cb);
    
    return 0;
}

void net_ne_timer_fini(net_timer_t base_timer) {
    net_ne_timer_t timer = net_timer_data(base_timer);
    dispatch_release(timer->m_timer);
    timer->m_timer = NULL;
}

void net_ne_timer_active(net_timer_t base_timer, uint32_t delay_ms) {
    net_ne_timer_t timer = net_timer_data(base_timer);
    dispatch_source_cancel(timer->m_timer);
    dispatch_source_set_timer(
        timer->m_timer, dispatch_time(DISPATCH_TIME_NOW, ((uint64_t)delay_ms) * 1000), DISPATCH_TIME_FOREVER, 1000ULL);
    dispatch_resume(timer->m_timer);
    timer->m_is_active = 1;
}

void net_ne_timer_cancel(net_timer_t base_timer) {
    net_ne_timer_t timer = net_timer_data(base_timer);
    dispatch_source_cancel(timer->m_timer);
    assert(timer->m_is_active == 0);
}

uint8_t net_ne_timer_is_active(net_timer_t base_timer) {
    net_ne_timer_t timer = net_timer_data(base_timer);
    return timer->m_is_active;
}

static void net_ne_timer_timeout_cb(void *arg) {
    net_ne_timer_t timer = arg;
    net_timer_t base_timer = net_timer_from_data(timer);

    timer->m_is_active = 0;
    net_timer_process(base_timer);
}

static void net_ne_timer_cancel_cb(void *arg) {
    net_ne_timer_t timer = arg;
    timer->m_is_active = 0;
}
