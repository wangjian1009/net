#ifndef NET_NDT7_RUNNER_H_INCLEDED
#define NET_NDT7_RUNNER_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_system.h"
#include "net_dns_system.h"
#include "net_ndt7_model.h"

struct ev_loop;
struct ev_signal;
typedef struct ndt7_runner * ndt7_runner_t;

struct ndt7_runner {
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

    net_ndt7_manage_t m_ndt_manager;
    net_ndt7_tester_t m_ndt_tester;

    struct mem_buffer m_data_buffer;
};

ndt7_runner_t ndt7_runner_create(mem_allocrator_t alloc);
void ndt7_runner_free(ndt7_runner_t runner);

/*主要操作 */
int ndt7_runner_init_net(ndt7_runner_t runner);
int ndt7_runner_init_dns(ndt7_runner_t runner);
int ndt7_runner_init_ndt(ndt7_runner_t runner);

void ndt7_runner_loop_run(ndt7_runner_t runner);
void ndt7_runner_loop_break(ndt7_runner_t runner);
int ndt7_runner_init_stop_sig(ndt7_runner_t runner, int sig);

/*测试 */
int ndt7_runner_start(ndt7_runner_t runner, net_ndt7_test_type_t test_type);

#endif
