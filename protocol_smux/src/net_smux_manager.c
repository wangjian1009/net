#include "cpe/pal/pal_stdio.h"
#include "net_schedule.h"
#include "net_smux_manager_i.h"
#include "net_smux_session_i.h"

net_smux_manager_t
net_smux_manager_create(net_schedule_t schedule, mem_allocrator_t alloc, error_monitor_t em) {
    net_smux_manager_t manager = mem_alloc(alloc, sizeof(struct net_smux_manager));

    manager->m_alloc = alloc;
    manager->m_em = em;
    manager->m_schedule = schedule;

    manager->m_cfg_version = 1;
    manager->m_cfg_keep_alive_disabled = 0;
    manager->m_cfg_keep_alive_interval_ms = 10 * 60 * 1000;
    manager->m_cfg_keep_alive_timeout_ms = 30 * 60 * 1000;
    manager->m_cfg_max_frame_size = 32768;
    manager->m_cfg_max_recv_buffer = 4194304;
    manager->m_cfg_max_stream_buffer = 65536;

    manager->m_max_session_id = 0;
    TAILQ_INIT(&manager->m_sessions);

    return manager;
}

void net_smux_manager_free(net_smux_manager_t manager) {

    while(!TAILQ_EMPTY(&manager->m_sessions)) {
        net_smux_session_free(TAILQ_FIRST(&manager->m_sessions));
    }
    
    mem_free(manager->m_alloc, manager);
}

mem_buffer_t net_smux_manager_tmp_buffer(net_smux_manager_t manager) {
    return net_schedule_tmp_buffer(manager->m_schedule);
}
