#ifndef NET_PING_TASK_I_H_INCLEDED
#define NET_PING_TASK_I_H_INCLEDED
#include "net_ping_task.h"
#include "net_ping_mgr_i.h"

struct net_ping_task {
    net_ping_mgr_t m_mgr;
    TAILQ_ENTRY(net_ping_task) m_next;
    net_ping_type_t m_type;
    uint8_t m_to_notify;
    uint8_t m_is_processing;
    uint8_t m_is_free;
    void * m_cb_ctx;
    net_ping_task_cb_fun_t m_cb_fun;
    union {
        struct {
            char * m_url;
        } m_http;
    };
    net_ping_task_state_t m_state;
    net_address_t m_target;
    uint16_t m_record_count;
    net_ping_record_list_t m_records;
    net_ping_processor_t m_processor;
};

void net_ping_task_real_free(net_ping_task_t task);
void net_ping_task_set_state(net_ping_task_t task, net_ping_task_state_t state);

void net_ping_task_set_to_notify(net_ping_task_t task, uint8_t to_notify);
void net_ping_task_do_notify(net_ping_task_t task);

#endif

