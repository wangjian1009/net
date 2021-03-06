#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_timer.h"
#include "net_driver.h"
#include "net_dq_timer.h"

int net_dq_timer_init(net_timer_t base_timer) {
    net_dq_timer_t timer = net_timer_data(base_timer);
    bzero((void*)timer, sizeof(*timer));
    return 0;
}

void net_dq_timer_fini(net_timer_t base_timer) {
    net_dq_timer_t timer = net_timer_data(base_timer);

    if (timer->m_timer) {
        dispatch_source_set_event_handler(timer->m_timer, nil);
        dispatch_source_cancel(timer->m_timer);
        timer->m_timer = nil;
    }
}

void net_dq_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds) {
    net_dq_timer_t timer = net_timer_data(base_timer);

    //net_dq_driver_t driver = net_driver_data(net_timer_driver(base_timer));
    //CPE_INFO(driver->m_em, "timer %p active, delay_ms=%d", timer, delay_ms);

    if (timer->m_timer) {
        dispatch_source_set_event_handler(timer->m_timer, nil);
        dispatch_source_cancel(timer->m_timer);
        timer->m_timer = nil;
    }

    dispatch_source_t cur_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());

    timer->m_timer = cur_timer;
    
    dispatch_source_set_event_handler(timer->m_timer, ^{
            if (timer->m_timer != cur_timer) return;

            dispatch_source_set_event_handler(cur_timer, nil);
            dispatch_source_cancel(cur_timer);
            timer->m_timer = nil;

            net_timer_process(base_timer);
        });

    dispatch_source_set_timer(
        timer->m_timer, dispatch_time(DISPATCH_TIME_NOW, (delay_milliseconds) * 1000u), 1ull * NSEC_PER_SEC, 0ULL);
    dispatch_resume(timer->m_timer);
}

void net_dq_timer_cancel(net_timer_t base_timer) {
    net_dq_timer_t timer = net_timer_data(base_timer);

    if (timer->m_timer) {
        dispatch_source_set_event_handler(timer->m_timer, nil);
        dispatch_source_cancel(timer->m_timer);
        timer->m_timer = nil;
    }
}

uint8_t net_dq_timer_is_active(net_timer_t base_timer) {
    net_dq_timer_t timer = net_timer_data(base_timer);
    return timer->m_timer ? 1 : 0;
}
