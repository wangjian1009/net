#ifndef NET_LOG_THREAD_H_INCLEDED
#define NET_LOG_THREAD_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

typedef int (*net_log_thread_init_fun_t)(void * ctx, net_log_thread_t log_thread);
typedef void (*net_log_thread_fini_fun_t)(void * ctx, net_log_thread_t log_thread);
typedef net_driver_t (*net_log_thread_setup_fun_t)(void * ctx, net_log_thread_t log_thread, net_schedule_t net_schedule);
typedef void (*net_log_thread_teardown_fun_t)(void * ctx, net_log_thread_t log_thread, net_schedule_t net_schedule);
typedef void (*net_log_thread_run_fun_t)(void * ctx, net_log_thread_t log_thread);
typedef void (*net_log_thread_stop_fun_t)(void * ctx, net_log_thread_t log_thread);

struct net_log_thread_processor {
    void * m_ctx;
    uint32_t m_capacity;
    net_log_thread_init_fun_t m_init;
    net_log_thread_fini_fun_t m_fini;
    net_log_thread_setup_fun_t m_setup;
    net_log_thread_teardown_fun_t m_teardown;
    net_log_thread_stop_fun_t m_stop;
    net_log_thread_run_fun_t m_run;
};
    
net_log_thread_t net_log_thread_create(
    net_log_schedule_t schedule, const char * name, net_log_thread_processor_t processor);

const char * net_log_thread_name(net_log_thread_t log_thread);
void * net_log_thread_data(net_log_thread_t log_thread);

NET_END_DECL

#endif
