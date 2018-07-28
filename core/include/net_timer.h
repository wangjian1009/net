#ifndef NET_TIMER_H_INCLEDED
#define NET_TIMER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef void (*net_timer_process_fun_t)(net_timer_t timer, void * ctx);

net_timer_t net_timer_create(
    net_driver_t driver,
    net_timer_process_fun_t process_fun, void * process_ctx);

net_timer_t net_timer_auto_create(
    net_schedule_t schedule,
    net_timer_process_fun_t process_fun, void * process_ctx);

void net_timer_free(net_timer_t timer);

net_schedule_t net_timer_schedule(net_timer_t timer);
net_driver_t net_timer_driver(net_timer_t timer);

void * net_timer_data(net_timer_t timer);
net_timer_t net_timer_from_data(void * data);

uint8_t net_timer_is_active(net_timer_t timer);
void net_timer_active(net_timer_t timer, int32_t delay_ms);
void net_timer_cancel(net_timer_t timer);

net_timer_process_fun_t net_timer_process_fun(net_timer_t timer);
void * net_timer_process_ctx(net_timer_t timer);
void net_timer_set_process_fun(net_timer_t timer, net_timer_process_fun_t process_fun, void * process_ctx);

void net_timer_process(net_timer_t timer);

NET_END_DECL

#endif
