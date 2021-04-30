#ifndef URL_RUNNER_H_INCLEDED
#define URL_RUNNER_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_system.h"
#include "net_dns_system.h"

struct ev_loop;
struct ev_signal;
typedef struct ndt_runner * ndt_runner_t;

struct ndt_runner {
    mem_allocrator_t m_alloc;
    FILE * m_log_file;
    struct error_monitor m_em_buf;
    error_monitor_t m_em;

    FILE * m_output;
    
    /*net*/
    struct ev_loop * m_event_base;
    uint8_t m_sig_event_count;
    uint8_t m_sig_event_capacity;
    struct ev_signal * m_sig_events;

    net_schedule_t m_net_schedule;
    net_dns_manage_t m_dns_manage;
    net_driver_t m_net_driver;
};

ndt_runner_t ndt_runner_create(mem_allocrator_t alloc);
void ndt_runner_free(ndt_runner_t runner);

/*主要操作 */
int ndt_runner_init_net(ndt_runner_t runner);
int ndt_runner_init_dns(ndt_runner_t runner);

void ndt_runner_loop_run(ndt_runner_t runner);
void ndt_runner_loop_break(ndt_runner_t runner);
int ndt_runner_init_stop_sig(ndt_runner_t runner, int sig);
    
#endif
