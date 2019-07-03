#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_timer.h"
#include "net_driver.h"
#include "net_android_timer.h"

int net_android_timer_init(net_timer_t base_timer) {
    net_android_timer_t timer = net_timer_data(base_timer);
    bzero((void*)timer, sizeof(*timer));
    return 0;
}

void net_android_timer_fini(net_timer_t base_timer) {
    net_android_timer_t timer = net_timer_data(base_timer);
}

void net_android_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds) {
    net_android_timer_t timer = net_timer_data(base_timer);

    //net_android_driver_t driver = net_driver_data(net_timer_driver(base_timer));
    //CPE_INFO(driver->m_em, "timer %p active, delay_ms=%d", timer, delay_ms);

}

void net_android_timer_cancel(net_timer_t base_timer) {
    net_android_timer_t timer = net_timer_data(base_timer);
}

uint8_t net_android_timer_is_active(net_timer_t base_timer) {
    net_android_timer_t timer = net_timer_data(base_timer);
    return 0;
}
