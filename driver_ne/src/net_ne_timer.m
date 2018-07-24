#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_timer.h"
#include "net_driver.h"
#include "net_ne_timer.h"

//static void net_ne_timer_cb(EV_P_ ev_timer *watcher, int revents);

int net_ne_timer_init(net_timer_t base_timer) {
    net_ne_timer_t timer = net_timer_data(base_timer);
    net_ne_driver_t driver = net_driver_data(net_timer_driver(base_timer));

    timer->m_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, driver->m_queue);
    //dispatch_retain(timeout_timer);
    
//    timeout_context->termTSR = startTSR + __CFTimeIntervalToTSR(seconds);
    dispatch_set_context(timer->m_timer, timer); // source gets ownership of context
    // dispatch_source_set_event_handler_f(timeout_timer, __CFRunLoopTimeout);
    // dispatch_source_set_cancel_handler_f(timeout_timer, __CFRunLoopTimeoutCancel);
    // uint64_t ns_at = (uint64_t)((__CFTSRToTimeInterval(startTSR) + seconds) * 1000000000ULL);
    // dispatch_source_set_timer(timeout_timer, dispatch_time(1, ns_at), DISPATCH_TIME_FOREVER, 1000ULL);
    // dispatch_resume(timeout_timer);
    
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
}

void net_ne_timer_cancel(net_timer_t base_timer) {
    net_ne_timer_t timer = net_timer_data(base_timer);
    dispatch_source_cancel(timer->m_timer);
}

uint8_t net_ne_timer_is_active(net_timer_t base_timer) {
    // net_ne_timer_t timer = net_timer_data(base_timer);
    // return ev_is_active(&timer->m_watcher);
    return 0;
}

// static void net_ne_timer_cb(EV_P_ ev_timer *watcher, int revents) {
//     net_ne_timer_t timer = watcher->data;
//     net_timer_t base_timer = net_timer_from_data(timer);
//     net_ne_driver_t driver = net_driver_data(net_timer_driver(base_timer));

//     ev_timer_stop(driver->m_ne_loop, &timer->m_watcher);

//     net_timer_process(base_timer);
// }
