#include "assert.h"
#include "net_dns_query.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_task_i.h"
#include "net_dns_task_builder_i.h"

int net_dns_query_ex_init(void * ctx, net_dns_query_t query, const char * hostname) {
    net_dns_manage_t manage = ctx;
    struct net_dns_query_ex * query_ex = net_dns_query_data(query);

    uint8_t is_entry_new = 0;
    uint8_t is_task_new = 0;

    net_dns_entry_t entry = net_dns_entry_find(manage, hostname);
    if (entry == NULL) {
        entry = net_dns_entry_create(manage, hostname);
        if (entry == NULL) return -1;
        is_entry_new = 1;
    }

    if (TAILQ_EMPTY(&entry->m_items)) {
        if (entry->m_task == NULL) {
            if (manage->m_builder_default == NULL) {
                CPE_ERROR(manage->m_em, "dns: query %s: no task builder!", hostname);
                goto START_ERROR;
            }
            
            if (net_dns_task_create(manage, entry) == NULL) {
                goto START_ERROR;
            }
            assert(entry->m_task != NULL);
            is_task_new = 1;

            if (net_dns_task_builder_build(manage->m_builder_default, entry->m_task) != 0) {
                CPE_ERROR(manage->m_em, "dns: query %s: build task fail!", hostname);
                goto START_ERROR;
            }

            if (net_dns_task_start(entry->m_task) != 0) {
                CPE_ERROR(manage->m_em, "dns: query %s: start task fail!", hostname);
                goto START_ERROR;
            }
        }
    }
    
    query_ex->m_manage = manage;
    query_ex->m_task = entry->m_task;
    query_ex->m_entry = NULL;

    if (query_ex->m_task) {
        TAILQ_INSERT_TAIL(&query_ex->m_task->m_querys, query_ex, m_next);
    }
    else {
        TAILQ_INSERT_TAIL(&query_ex->m_manage->m_to_notify_querys, query_ex, m_next);
        net_dns_manage_active_delay_process(manage);
    }
    
    return 0;

START_ERROR:
    if (is_entry_new) {
        net_dns_entry_free(entry);
    }
    else if (is_task_new) {
        net_dns_task_free(entry->m_task);
        assert(entry->m_task == NULL);
    }

    return -1;
}

void net_dns_query_ex_fini(void * ctx, net_dns_query_t query) {
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
    if (query_ex->m_task == task) return;

    net_dns_manage_t manage = query_ex->m_manage;
    
    if (query_ex->m_task) {
        TAILQ_REMOVE(&query_ex->m_task->m_querys, query_ex, m_next);
    }
    else {
        TAILQ_REMOVE(&manage->m_to_notify_querys, query_ex, m_next);
    }

    query_ex->m_task = task;

    if (query_ex->m_task) {
        TAILQ_INSERT_TAIL(&query_ex->m_task->m_querys, query_ex, m_next);
    }
    else {
        TAILQ_INSERT_TAIL(&manage->m_to_notify_querys, query_ex, m_next);
        net_dns_manage_active_delay_process(manage);
    }
}
