#include "net_dns_query.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_task_i.h"

int net_dns_query_ex_start(void * ctx, net_dns_query_t query, const char * hostname) {
    net_dns_manage_t manage = ctx;
    struct net_dns_query_ex * query_ex = net_dns_query_data(query);

    net_dns_task_t task = NULL;

    net_dns_entry_t entry = net_dns_entry_find(manage, hostname);
    if (entry == NULL) {
        entry = net_dns_entry_create(manage, hostname, NULL, 0, 0);
        if (entry == NULL) return -1;
    }
    else {
        task = net_dns_task_find(manage, entry);
    }

    if (entry->m_address == NULL) {
        if (task == NULL) {
        }
    }

    if (query_ex->m_task) {
        
    }
    else {
    }
    
    return 0;
}

void net_dns_query_ex_cancel(void * ctx, net_dns_query_t query) {
    net_dns_manage_t manage = ctx;
    struct net_dns_query_ex * query_ex = net_dns_query_data(query);

    if (query_ex->m_task) {
        TAILQ_REMOVE(&query_ex->m_task->m_querys, query_ex, m_next);
    }
    else {
        TAILQ_REMOVE(&query_ex->m_manage->m_to_notify_querys, query_ex, m_next);
    }
}

void net_dns_query_ex_set_task(net_dns_query_ex_t query_ex, net_dns_task_t task) {
    if (query_ex->m_task) {
        TAILQ_REMOVE(&query_ex->m_task->m_querys, query_ex, m_next);
    }
    else {
        TAILQ_REMOVE(&query_ex->m_manage->m_to_notify_querys, query_ex, m_next);
    }

    query_ex->m_task = task;

    if (query_ex->m_task) {
        TAILQ_INSERT_TAIL(&query_ex->m_task->m_querys, query_ex, m_next);
    }
    else {
        TAILQ_INSERT_TAIL(&query_ex->m_manage->m_to_notify_querys, query_ex, m_next);
    }
}
