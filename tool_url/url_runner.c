#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "event2/event.h"
#include "net_driver.h"
#include "net_libevent_driver.h"
#include "net_schedule.h"
#include "url_runner.h"

url_runner_t
url_runner_create(mem_allocrator_t alloc) {
    url_runner_t runner = mem_alloc(alloc, sizeof(struct url_runner));
    if (runner == NULL) {
        printf("alloc colsole fail!");
        return NULL;
    }
    bzero(runner, sizeof(*runner));

    runner->m_alloc = alloc;
    cpe_error_monitor_init(&runner->m_em_buf, cpe_error_log_to_consol, NULL);
    runner->m_em = &runner->m_em_buf;
    runner->m_sig_event_count = 0;

    runner->m_mode = url_runner_mode_init;
    return runner;
}


void url_runner_free(url_runner_t runner) {
    url_runner_set_mode(runner, url_runner_mode_init);

    if (runner->m_net_driver) {
        net_driver_free(runner->m_net_driver);
        runner->m_net_driver = NULL;
    }
    
    if (runner->m_net_schedule) {
        net_schedule_free(runner->m_net_schedule);
        runner->m_net_schedule = NULL;
    }

    uint8_t i;
    for(i = 0; i < runner->m_sig_event_count; ++i) {
        assert(runner->m_sig_events[i]);
        evsignal_del(runner->m_sig_events[i]);
        runner->m_sig_events[i] = NULL;
    }
    runner->m_sig_event_count = 0;

    if (runner->m_event_base) {
        event_base_free(runner->m_event_base);
        runner->m_event_base = NULL;
    }
    
    mem_free(runner->m_alloc, runner);
}

void url_runner_loop_run(url_runner_t entry_runner) {
    event_base_loop(entry_runner->m_event_base, 0);
}

void url_runner_loop_break(url_runner_t entry_runner) {
    event_base_loopbreak(entry_runner->m_event_base);
}

void url_runner_stop_signal_cb(evutil_socket_t signo, short events, void* arg) {
    url_runner_t entry_runner = arg;
    //TODO:
}

int url_runner_init_stop_sig(url_runner_t entry_runner, int sig) {
    assert(entry_runner->m_sig_event_count < CPE_ARRAY_SIZE(entry_runner->m_sig_events));

    struct event * sigbreak_event = evsignal_new(entry_runner->m_event_base, sig, url_runner_stop_signal_cb, entry_runner);
    evsignal_add(sigbreak_event, NULL);

    entry_runner->m_sig_events[entry_runner->m_sig_event_count] = sigbreak_event;
    entry_runner->m_sig_event_count++;
    return 0;
}

int url_runner_init_net(url_runner_t runner) {
    assert(runner->m_event_base == NULL);
    runner->m_event_base = event_base_new();
    if (runner->m_event_base == NULL) {
        CPE_ERROR(runner->m_em, "entry: create event loop fail!");
        return -1;
    }

    assert(runner->m_net_schedule == NULL);
    runner->m_net_schedule = net_schedule_create(runner->m_alloc, runner->m_em, NULL);
    if (runner->m_net_schedule == NULL) {
        CPE_ERROR(runner->m_em, "entry: create schedule fail!");
        event_base_free(runner->m_event_base);
        runner->m_event_base = NULL;
        return -1;
    }

    net_libevent_driver_t libevent_driver = net_libevent_driver_create(runner->m_net_schedule, runner->m_event_base);
    if (libevent_driver == NULL) {
        CPE_ERROR(runner->m_em, "entry: create driver fail!");
        net_schedule_free(runner->m_net_schedule);
        runner->m_net_schedule = NULL;
        event_base_free(runner->m_event_base);
        runner->m_event_base = NULL;
        return -1;
    }

    assert(runner->m_net_driver == NULL);
    runner->m_net_driver = net_libevent_driver_base_driver(libevent_driver);
    //net_driver_set_debug(schedule->m_net_driver, 2);
    //net_schedule_set_direct_driver(entry_runner->m_net_schedule, entry_runner->m_net_driver);

    return 0;
}

int url_runner_set_mode(url_runner_t runner, url_runner_mode_t mode) {
    if (runner->m_mode == mode) return 0;

    switch(runner->m_mode) {
    case url_runner_mode_init:
        break;
    case url_runner_mode_internal:
        url_runner_internal_fini(runner);
        break;
    }

    runner->m_mode = mode;
    
    switch(runner->m_mode) {
    case url_runner_mode_init:
        break;
    case url_runner_mode_internal:
        if (url_runner_internal_init(runner) != 0) {
            runner->m_mode = url_runner_mode_init;
            return -1;
        }
        break;
    }

    return 0;
}

int url_runner_start(url_runner_t runner, const char * method, const char * url, const char * body) {
    switch (runner->m_mode) {
    case url_runner_mode_init:
        CPE_ERROR(runner->m_em, "not support start in mode init");
        return -1;
    case url_runner_mode_internal:
        return url_runner_internal_start(runner, method, url, body);
    }
}
