#ifndef NET_DRIVER_I_H_INCLEDED
#define NET_DRIVER_I_H_INCLEDED
#include "net_driver.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_driver {
    net_schedule_t m_schedule;
    TAILQ_ENTRY(net_driver) m_next_for_schedule;
    char m_name[64];
    uint8_t m_debug;

    /*driver*/
    uint16_t m_driver_capacity;
    net_driver_init_fun_t m_driver_init;
    net_driver_fini_fun_t m_driver_fini;
    net_driver_time_fun_t m_driver_time;

    /*timer*/
    uint16_t m_timer_capacity;
    net_timer_init_fun_t m_timer_init;
    net_timer_fini_fun_t m_timer_fini;
    net_timer_schedule_fun_t m_timer_schedule;
    net_timer_cancel_fun_t m_timer_cancel;
    net_timer_is_active_fun_t m_timer_is_active;

    /*acceptor*/
    uint16_t m_acceptor_capacity;
    net_acceptor_init_fun_t m_acceptor_init;
    net_acceptor_fini_fun_t m_acceptor_fini;

    /*endpoint*/
    uint16_t m_endpoint_capacity;
    net_endpoint_init_fun_t m_endpoint_init;
    net_endpoint_fini_fun_t m_endpoint_fini;
    net_endpoint_calc_size_fun_t m_endpoint_calc_size;
    net_endpoint_connect_fun_t m_endpoint_connect;
    net_endpoint_update_fun_t m_endpoint_update;
    net_endpoint_set_no_delay_fun_t m_endpoint_set_no_delay;
    net_endpoint_get_mss_fun_t m_endpoint_get_mss;

    /*dgram*/
    uint16_t m_dgram_capacity;
    net_dgram_init_fun_t m_dgram_init;
    net_dgram_fini_fun_t m_dgram_fini;
    net_dgram_send_fun_t m_dgram_send;

    /*watcher*/
    uint16_t m_watcher_capacity;
    net_watcher_init_fun_t m_watcher_init;
    net_watcher_fini_fun_t m_watcher_fini;
    net_watcher_update_fun_t m_watcher_update;

    /*progress*/
    uint16_t m_progress_capacity;
    net_progress_init_fun_t m_progress_init;
    net_progress_fini_fun_t m_progress_fini;
    
    /*runtime*/
    net_acceptor_list_t m_acceptors;
    net_acceptor_list_t m_free_acceptors;

    net_endpoint_list_t m_endpoints;
    net_endpoint_list_t m_deleting_endpoints;
    net_endpoint_list_t m_free_endpoints;

    net_dgram_list_t m_dgrams;
    net_dgram_list_t m_free_dgrams;

    net_timer_list_t m_timers;
    net_timer_list_t m_free_timers;

    net_watcher_list_t m_watchers;
    net_watcher_list_t m_free_watchers;

    net_progress_list_t m_progresses;
};

NET_END_DECL

#endif
