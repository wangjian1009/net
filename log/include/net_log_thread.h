#ifndef NET_LOG_THREAD_H_INCLEDED
#define NET_LOG_THREAD_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

typedef net_driver_t (*net_log_thread_setup_fun_t)(void * ctx, net_log_thread_t thread, net_schedule_t net_schedule);
typedef void (*net_log_thread_teardown_fun_t)(void * ctx, net_log_thread_t thread, net_schedule_t net_schedule);
typedef void (*net_log_thread_run_fun_t)(void * ctx, net_schedule_t net_schedule);
typedef void (*net_log_thread_stop_fun_t)(void * ctx, net_schedule_t net_schedule);
typedef void (*net_log_thread_ctx_fini_fun_t)(void * ctx);

struct net_log_thread_processor {
    void * m_ctx;
    uint32_t m_capacity;
    int (*m_init)(void * ctx, net_log_thread_t log_thread);
    void (*m_fini)(void * ctx, net_log_thread_t log_thread);
    net_driver_t (*m_setup)(void * ctx, net_log_thread_t thread, net_schedule_t net_schedule);
    void (*m_teardown)(void * ctx, net_log_thread_t thread, net_schedule_t net_schedule);
    void (*m_stop)(void * ctx, net_log_thread_t thread);
    void (*m_run)(void * ctx, net_log_thread_t thread);
};
    
net_log_thread_t net_log_thread_create(
    net_log_schedule_t schedule, const char * name, net_log_thread_processor_t processor);

NET_END_DECL

#endif
