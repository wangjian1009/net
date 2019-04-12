#include "assert.h"
#include "cpe/utils/time_utils.h"
#include "net_dns_task_i.h"
#include "net_dns_task_step_i.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_entry_i.h"

net_dns_task_t net_dns_task_create(net_dns_manage_t manage, net_dns_entry_t entry, net_dns_query_type_t query_type) {
    net_dns_task_t task;

    assert(entry->m_task == NULL);
    
    task = TAILQ_FIRST(&manage->m_free_tasks);
    if (task) {
        TAILQ_REMOVE(&manage->m_free_tasks, task, m_next);
    }
    else {
        task = mem_alloc(manage->m_alloc, sizeof(struct net_dns_task));
        if (task == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: task alloc fail!");
            return NULL;
        }
    }

    task->m_manage = manage;
    task->m_query_type = query_type;
    task->m_entry = entry;
    task->m_state = net_dns_task_state_init;
    task->m_step_current = NULL;
    task->m_begin_time_ms = 0;
    task->m_complete_time_ms = 0;
    TAILQ_INIT(&task->m_steps);
    TAILQ_INIT(&task->m_querys);
    
    entry->m_task = task;

    TAILQ_INSERT_TAIL(&manage->m_runing_tasks, task, m_next);
    
    if (manage->m_debug >= 2) {
        CPE_INFO(manage->m_em, "dns-cli: query %s: start!", entry->m_hostname);
    }
    
    return task;
}

void net_dns_task_free(net_dns_task_t task) {
    net_dns_manage_t manage = task->m_manage;

    if (manage->m_debug >= 2) {
        CPE_INFO(manage->m_em, "dns-cli: query %s: free!", task->m_entry->m_hostname);
    }

    assert(task->m_entry->m_task == task);
    task->m_entry->m_task = NULL;

    while(!TAILQ_EMPTY(&task->m_steps)) {
        net_dns_task_step_free(TAILQ_FIRST(&task->m_steps));
    }
    
    while(!TAILQ_EMPTY(&task->m_querys)) {
        net_dns_query_ex_set_task(TAILQ_FIRST(&task->m_querys), NULL);
    }

    if (net_dns_task_is_complete(task)) {
        TAILQ_REMOVE(&manage->m_complete_tasks, task, m_next);
    }
    else {
        TAILQ_REMOVE(&manage->m_runing_tasks, task, m_next);
    }

    TAILQ_INSERT_TAIL(&manage->m_free_tasks, task, m_next);
}

void net_dns_task_real_free(net_dns_task_t task) {
    net_dns_manage_t manage = task->m_manage;

    TAILQ_REMOVE(&manage->m_free_tasks, task, m_next);
    mem_free(manage->m_alloc, task);
}

const char * net_dns_task_hostname(net_dns_task_t task) {
    return task->m_entry->m_hostname;
}

net_dns_query_type_t net_dns_task_query_type(net_dns_task_t task) {
    return task->m_query_type;
}

int net_dns_task_start(net_dns_task_t task) {
    net_dns_manage_t manage = task->m_manage;

    if (task->m_state != net_dns_task_state_init) {
        CPE_ERROR(
            manage->m_em,
            "dns-cli: query %s: can`t start in state %s",
            task->m_entry->m_hostname,
            net_dns_task_state_str(task->m_state));
        return -1;
    }

    assert(task->m_step_current == NULL);
    
    task->m_step_current = TAILQ_FIRST(&task->m_steps);
    if (task->m_step_current == NULL) {
        if (manage->m_debug) {
            CPE_INFO(
                manage->m_em, "dns-cli: query %s: no step", task->m_entry->m_hostname);
        }
        net_dns_task_update_state(task, net_dns_task_state_success);
        return 0;
    }

    do {
        net_dns_task_step_start(task->m_step_current);

        switch(net_dns_task_step_state(task->m_step_current)) {
        case net_dns_task_state_init:
            CPE_ERROR(
                manage->m_em, "dns-cli: query %s: step start but still init",
                task->m_entry->m_hostname);
            net_dns_task_update_state(task, net_dns_task_state_error);
            return -1;
        case net_dns_task_state_runing:
            net_dns_task_update_state(task, net_dns_task_state_runing);
            return 0;
        case net_dns_task_state_success:
            return 0;
        case net_dns_task_state_empty:
            break;
        case net_dns_task_state_error:
            break;
        }
    
        net_dns_task_step_t next_step = TAILQ_NEXT(task->m_step_current, m_next);
        if (next_step) {
            task->m_step_current = next_step;
            continue;
        }
        else {
            return -1;
        }
    } while(1);
}

net_dns_task_step_t net_dns_task_step_current(net_dns_task_t task) {
    return task->m_step_current;
}

static net_dns_task_step_t net_dns_task_step_next(struct net_dns_task_step_it * it) {
    net_dns_task_step_t * data = (net_dns_task_step_t *)(it->m_data);
    net_dns_task_step_t r;
    if (*data == NULL) return NULL;
    r = *data;
    *data = TAILQ_NEXT(r, m_next);
    return r;
}

void net_dns_task_steps(net_dns_task_t task, net_dns_task_step_it_t it) {
    *(net_dns_task_step_t *)(it->m_data) = TAILQ_FIRST(&task->m_steps);
    it->next = net_dns_task_step_next;
}

net_dns_task_state_t net_dns_task_state(net_dns_task_t task) {
    return task->m_state;
}

uint8_t net_dns_task_is_complete(net_dns_task_t task) {
    return net_dns_task_state_is_complete(task->m_state);
}

void net_dns_task_update_state(net_dns_task_t task, net_dns_task_state_t new_state) {
    if (new_state == task->m_state) return;

    net_dns_manage_t manage = task->m_manage;
    if (manage->m_debug >= 2) {
        CPE_INFO(
            manage->m_em, "dns-cli: query %s: state %s ==> %s",
            task->m_entry->m_hostname,
            net_dns_task_state_str(task->m_state),
            net_dns_task_state_str(new_state));
    }

    uint8_t old_is_complete = net_dns_task_is_complete(task);
    task->m_state = new_state;
    uint8_t new_is_complete = net_dns_task_is_complete(task);

    if (new_state == net_dns_task_state_runing) {
        task->m_begin_time_ms = cur_time_ms();
    }
    
    if (old_is_complete == new_is_complete) return;
    
    if (new_is_complete) {
        task->m_complete_time_ms = cur_time_ms();
        if (task->m_begin_time_ms == 0) {
            task->m_begin_time_ms = task->m_complete_time_ms;
        }
        TAILQ_REMOVE(&manage->m_runing_tasks, task, m_next);
        TAILQ_INSERT_TAIL(&manage->m_complete_tasks, task, m_next);
        net_dns_manage_active_delay_process(manage);
    }
    else {
        TAILQ_REMOVE(&manage->m_complete_tasks, task, m_next);
        TAILQ_INSERT_TAIL(&manage->m_runing_tasks, task, m_next);
    }
}

net_dns_task_state_t net_dns_task_calc_state(net_dns_task_t task) {
    if (task->m_step_current) {
        switch(net_dns_task_step_state(task->m_step_current)) {
        case net_dns_task_state_init:
        case net_dns_task_state_runing:
            return net_dns_task_state_runing;
        case net_dns_task_state_success:
            return net_dns_task_state_success;
        case net_dns_task_state_empty:
            return TAILQ_NEXT(task->m_step_current, m_next) ? net_dns_task_state_runing : net_dns_task_state_empty;
        case net_dns_task_state_error:
            return TAILQ_NEXT(task->m_step_current, m_next) ? net_dns_task_state_runing : net_dns_task_state_error;
        }
    }
    else {
        return TAILQ_EMPTY(&task->m_steps) ? net_dns_task_state_error : net_dns_task_state_empty;
    }
}

const char * net_dns_task_state_str(net_dns_task_state_t state) {
    switch(state) {
    case net_dns_task_state_init:
        return "init";
    case net_dns_task_state_runing:
        return "runing";
    case net_dns_task_state_success:
        return "success";
    case net_dns_task_state_empty:
        return "empty";
    case net_dns_task_state_error:
        return "error";
    }
}

uint8_t net_dns_task_state_is_complete(net_dns_task_state_t state) {
    switch(state) {
    case net_dns_task_state_success:
    case net_dns_task_state_error:
    case net_dns_task_state_empty:
        return 1;
    default:
        return 0;
    }
}

int64_t net_dns_task_begin_time_ms(net_dns_task_t task) {
    return task->m_begin_time_ms;
}

int64_t net_dns_task_complete_time_ms(net_dns_task_t task) {
    return task->m_complete_time_ms;
}

