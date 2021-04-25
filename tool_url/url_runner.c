#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils_sock/getdnssvraddrs.h"
#include "net_driver.h"
#include "net_address.h"
#include "net_ev_driver.h"
#include "net_schedule.h"
#include "net_dns_manage.h"
#include "net_dns_udns_source.h"
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
    runner->m_log_file = fopen("log.txt", "wb");
    if (runner->m_log_file) {
        cpe_error_monitor_init(&runner->m_em_buf, cpe_error_log_to_file, runner->m_log_file);
    }
    else {
        cpe_error_monitor_init(&runner->m_em_buf, cpe_error_log_to_consol, NULL);
    }

    runner->m_output = stdout;
    
    runner->m_em = &runner->m_em_buf;
    runner->m_sig_event_count = 0;
    runner->m_sig_event_capacity = 0;
    runner->m_sig_events = NULL;

    runner->m_mode = url_runner_mode_init;

    runner->m_sig_event_capacity = 8;
    runner->m_sig_events = mem_alloc(runner->m_alloc, sizeof(struct ev_signal) * runner->m_sig_event_capacity);
    if (runner->m_sig_events == NULL) {
        url_runner_free(runner);
        return NULL;
    }
    
    return runner;
}


void url_runner_free(url_runner_t runner) {
    url_runner_set_mode(runner, url_runner_mode_init);

    if (runner->m_dns_manage) {
        net_dns_manage_free(runner->m_dns_manage);
        runner->m_dns_manage = NULL;
    }

    if (runner->m_net_driver) {
        net_driver_free(runner->m_net_driver);
        runner->m_net_driver = NULL;
    }

    if (runner->m_net_schedule) {
        net_schedule_free(runner->m_net_schedule);
        runner->m_net_schedule = NULL;
    }

    if (runner->m_sig_events) {
        uint8_t i;
        for (i = 0; i < runner->m_sig_event_count; ++i) {
            //struct event * sigbreak_event = evsignal_new(runner->m_event_base, sig, sfox_runner_stop_signal_cb, runner);
            ev_signal_stop(runner->m_event_base, &runner->m_sig_events[i]);
        }
        mem_free(runner->m_alloc, runner->m_sig_events);
        runner->m_sig_event_capacity = 0;
    }
    runner->m_sig_event_count = 0;

    if (runner->m_event_base) {
        ev_loop_destroy(runner->m_event_base);
        runner->m_event_base = NULL;
    }

    if (runner->m_log_file) {
        fclose(runner->m_log_file);
        runner->m_log_file = NULL;
    }

    mem_free(runner->m_alloc, runner);
}

void url_runner_loop_run(url_runner_t runner) {
    ev_run(runner->m_event_base, 0);
}

void url_runner_loop_break(url_runner_t runner) {
    ev_break(runner->m_event_base, 0);
}

void sfox_runner_stop_signal_cb(EV_P_ ev_signal *watcher, int revents) {
    url_runner_t runner = watcher->data;
    ev_break(runner->m_event_base, 0);
}

int url_runner_init_stop_sig(url_runner_t runner, int sig) {
    assert(runner->m_sig_event_count < runner->m_sig_event_capacity);

    ev_signal_init(
        &runner->m_sig_events[runner->m_sig_event_count],
        sfox_runner_stop_signal_cb,
        sig);
    runner->m_sig_events[runner->m_sig_event_count].data = runner;
    
    runner->m_sig_event_count++;
    return 0;
}

int url_runner_init_net(url_runner_t runner) {
    assert(runner->m_event_base == NULL);
    runner->m_event_base = ev_loop_new(0);
    if (runner->m_event_base == NULL) {
        CPE_ERROR(runner->m_em, "entry: create event loop fail!");
        return -1;
    }

    assert(runner->m_net_schedule == NULL);
    runner->m_net_schedule = net_schedule_create(runner->m_alloc, runner->m_em, NULL);
    if (runner->m_net_schedule == NULL) {
        CPE_ERROR(runner->m_em, "entry: create schedule fail!");
        ev_loop_destroy(runner->m_event_base);
        runner->m_event_base = NULL;
        return -1;
    }

    runner->m_dns_manage = net_dns_manage_create(runner->m_alloc, runner->m_em, runner->m_net_schedule);
    if (runner->m_dns_manage == NULL) {
        CPE_ERROR(runner->m_em, "entry: create driver fail!");
        net_schedule_free(runner->m_net_schedule);
        runner->m_net_schedule = NULL;
        ev_loop_destroy(runner->m_event_base);
        runner->m_event_base = NULL;
        return -1;
    }

    net_ev_driver_t ev_driver = net_ev_driver_create(runner->m_net_schedule, runner->m_event_base);
    if (ev_driver == NULL) {
        CPE_ERROR(runner->m_em, "entry: create driver fail!");
        net_dns_manage_free(runner->m_dns_manage);
        runner->m_dns_manage = NULL;
        net_schedule_free(runner->m_net_schedule);
        runner->m_net_schedule = NULL;
        ev_loop_destroy(runner->m_event_base);
        runner->m_event_base = NULL;
        return -1;
    }

    assert(runner->m_net_driver == NULL);
    runner->m_net_driver = net_ev_driver_base_driver(ev_driver);
    //net_driver_set_debug(schedule->m_net_driver, 2);
    //net_schedule_set_direct_driver(entry_runner->m_net_schedule, entry_runner->m_net_driver);

    return 0;
}

int url_runner_init_dns(url_runner_t runner) {
    struct sockaddr_storage dnssevraddrs[10];
    uint8_t addr_count = CPE_ARRAY_SIZE(dnssevraddrs);
    if (getdnssvraddrs(dnssevraddrs, &addr_count, runner->m_em) != 0) {
        CPE_ERROR(runner->m_em, "url: init dns: get dns address fail");
        return -1;
    }

    net_dns_udns_source_t udns =
        net_dns_udns_source_create(
            runner->m_alloc, runner->m_em, 0,
            runner->m_dns_manage, runner->m_net_driver);
    if (udns == NULL) {
        CPE_ERROR(runner->m_em, "url: init dns: init udns fail");
        return -1;
    }

    uint8_t i;
    for(i = 0; i < addr_count; i++) {
        net_address_t address = net_address_create_from_sockaddr(
            runner->m_net_schedule, (struct sockaddr *)&dnssevraddrs[i], sizeof(dnssevraddrs[i]));
        if (address == NULL) {
            CPE_ERROR(runner->m_em, "url: init: dns: create address from sock addr fail");
            return -1;
        }

        if (net_dns_udns_source_add_server(udns, address) != 0){
            CPE_ERROR(
                runner->m_em, "url: init: dns: add %s udns fail",
                net_address_dump(net_schedule_tmp_buffer(runner->m_net_schedule), address));
            net_address_free(address);
            return -1;
        }

        CPE_INFO(
            runner->m_em, "url: init: dns: add %s to udns success",
                net_address_dump(net_schedule_tmp_buffer(runner->m_net_schedule), address));

        net_address_free(address);
    }

    if (net_dns_udns_source_start(udns) != 0) {
        CPE_ERROR(runner->m_em, "url: init: dns: udns start fail");
        net_dns_udns_source_free(udns);
        return -1;
    }

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

int url_runner_start(
    url_runner_t runner,
    const char * method, const char * url,
    const char * header[], uint16_t header_count, const char * body)
{
    switch (runner->m_mode) {
    case url_runner_mode_init:
        CPE_ERROR(runner->m_em, "not support start in mode init");
        return -1;
    case url_runner_mode_internal:
        return url_runner_internal_start(runner, method, url, header, header_count, body);
    }
}
