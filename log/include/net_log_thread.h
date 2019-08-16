#ifndef NET_LOG_THREAD_H_INCLEDED
#define NET_LOG_THREAD_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

typedef net_driver_t (*net_log_thread_setup_fun_t)(void * ctx, net_log_thread_t thread, net_schedule_t net_schedule);
typedef void (*net_log_thread_teardown_fun_t)(void * ctx, net_log_thread_t thread, net_schedule_t net_schedule);
typedef void (*net_log_thread_run_fun_t)(void * ctx, net_schedule_t net_schedule);
typedef void (*net_log_thread_stop_fun_t)(void * ctx, net_schedule_t net_schedule);
typedef void (*net_log_thread_ctx_fini_fun_t)(void * ctx);

net_log_thread_t net_log_thread_create(
    net_log_schedule_t schedule, const char * name,
    void * ctx,
    net_log_thread_setup_fun_t setup,
    net_log_thread_teardown_fun_t teardown,
    net_log_thread_run_fun_t run,
    net_log_thread_stop_fun_t stop,
    net_log_thread_ctx_fini_fun_t ctx_fini);

NET_END_DECL

#endif
