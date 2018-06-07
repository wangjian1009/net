#include "net_dns_task_i.h"

net_dns_task_t net_dns_task_create(net_dns_manage_t manage, net_dns_entry_t entry) {
    net_dns_task_t task;

    task = TAILQ_FIRST(&manage->m_free_tasks);
    if (task) {
        TAILQ_REMOVE(&manage->m_free_tasks, task, m_next);
    }
    else {
        task = mem_alloc(manage->m_alloc, sizeof(struct net_dns_task));
        if (task == NULL) {
            CPE_ERROR(manage->m_em, "dns: task alloc fail!");
            return NULL;
        }
    }

    task->m_manage = manage;
    task->m_entry = entry;

    cpe_hash_entry_init(&task->m_hh);
    if (cpe_hash_table_insert_unique(&manage->m_tasks, task) != 0) {
        CPE_ERROR(manage->m_em, "dns: task duplicate!");
        TAILQ_INSERT_TAIL(&manage->m_free_tasks, task, m_next);
        return NULL;
    }
    
    return task;
}

void net_dns_task_free(net_dns_task_t task) {
    net_dns_manage_t manage = task->m_manage;

    cpe_hash_table_remove_by_ins(&manage->m_tasks, task);

    TAILQ_INSERT_TAIL(&manage->m_free_tasks, task, m_next);
}

void net_dns_task_real_free(net_dns_task_t task) {
    net_dns_manage_t manage = task->m_manage;

    TAILQ_REMOVE(&manage->m_free_tasks, task, m_next);
    mem_free(manage->m_alloc, task);
}

void net_dns_task_free_all(net_dns_manage_t manage) {
    struct cpe_hash_it task_it;
    net_dns_task_t task;

    cpe_hash_it_init(&task_it, &manage->m_tasks);

    task = cpe_hash_it_next(&task_it);
    while(task) {
        net_dns_task_t next = cpe_hash_it_next(&task_it);
        net_dns_task_free(task);
        task = next;
    }
}

net_dns_task_t
net_dns_task_find(net_dns_manage_t manage, net_dns_entry_t entry) {
    struct net_dns_task key;
    key.m_entry = entry;
    return cpe_hash_table_find(&manage->m_tasks, &key);
}

uint32_t net_dns_task_hash(net_dns_task_t o) {
    return (uint32_t)(ptr_int_t)o->m_entry;
}

int net_dns_task_eq(net_dns_task_t l, net_dns_task_t r) {
    return l->m_entry == r->m_entry ? 1 : 0;
}

