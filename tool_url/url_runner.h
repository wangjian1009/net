#ifndef URL_RUNNER_H_INCLEDED
#define URL_RUNNER_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_system.h"

typedef struct url_runner * url_runner_t;

struct url_runner {
    mem_allocrator_t m_alloc;
    struct error_monitor m_em_buf;
    error_monitor_t m_em;

    /*net*/
    struct event_base * m_event_base;
    uint8_t m_sig_event_count;
    struct event * m_sig_events[8];

    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
};

url_runner_t url_runner_create(mem_allocrator_t alloc);
void url_runner_free(url_runner_t runner);

int url_runner_init_net(url_runner_t runner);

void url_runner_loop_run(url_runner_t entry_runner);
void url_runner_loop_break(url_runner_t entry_runner);
int url_runner_init_stop_sig(url_runner_t entry_runner, int sig);

#endif
