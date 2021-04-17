#ifndef NET_PROGRESS_I_H_INCLEDED
#define NET_PROGRESS_I_H_INCLEDED
#include "net_progress.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_progress {
    net_driver_t m_driver;
    uint32_t m_id;
    char * m_cmd;
    TAILQ_ENTRY(net_progress) m_next_for_driver;
    net_progress_runing_mode_t m_mode;
    net_mem_group_t m_mem_group;
    void * m_update_ctx;
    net_progress_update_fun_t m_update_fun;
    net_progress_state_t m_state;
    net_mem_block_t m_tb;
    net_mem_block_list_t m_blocks;
    uint32_t m_block_size;
};

NET_END_DECL

#endif
