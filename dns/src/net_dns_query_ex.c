#include "assert.h"
#include "cpe/utils/time_utils.h"
#include "net_address.h"
#include "net_dns_query.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_task_i.h"
#include "net_dns_task_builder_i.h"

static int net_dns_query_ex_init_for_dns(
    net_dns_manage_t manage, struct net_dns_query_ex * query_ex,
    net_address_t address, net_dns_query_type_t query_type, const char * policy)
{
    uint8_t have_response = 0;
    net_dns_entry_t entry = NULL;
    net_dns_task_t task = NULL;
    uint8_t is_entry_new = 0;
    uint8_t is_task_new = 0;

    assert(query_type != net_dns_query_domain);
    assert(net_address_type(address) == net_address_domain);

    const char * hostname = (const char *)net_address_data(address);
    assert(hostname);

    entry = net_dns_entry_find(manage, address);
    if (entry == NULL) {
        entry = net_dns_entry_create(manage, address);
        if (entry == NULL) return -1;
        is_entry_new = 1;
    }
    assert(entry);

    if (net_dns_entry_select_item(entry, manage->m_default_item_select_policy, query_type) != NULL) {
        have_response = 1;
    }

    uint8_t expired =
        (entry->m_expire_time_s == 0
         || entry->m_expire_time_s < (uint32_t)(cur_time_ms() / 1000))
        ? 1 : 0;
    
    if (!have_response || expired) {
        TAILQ_FOREACH(task, &entry->m_tasks, m_next_for_entry) {
            if (task->m_query_type == query_type) break;
        }
            
        if (task == NULL) {
            if (manage->m_builder_default == NULL) {
                CPE_ERROR(manage->m_em, "dns-cli: query %s: no task builder!", hostname);
                goto START_ERROR;
            }

            task = net_dns_task_create_for_entry(manage, entry, query_type);
            if (task == NULL) {
                goto START_ERROR;
            }
            is_task_new = 1;

            if (net_dns_task_builder_build(manage->m_builder_default, task, policy) != 0) {
                CPE_ERROR(manage->m_em, "dns-cli: query %s: build task fail!", hostname);
                goto START_ERROR;
            }

            if (net_dns_task_start(task) != 0) {
                CPE_ERROR(manage->m_em, "dns-cli: query %s: start task fail!", hostname);
                goto START_ERROR;
            }
        }
        else {
            if (manage->m_debug) {
                CPE_INFO(manage->m_em, "dns-cli: query %s: already have task!", hostname);
            }
        }
    }
    
    query_ex->m_manage = manage;
    query_ex->m_query_type = query_type;

    if (!have_response) {
        assert(task != NULL);
        
        query_ex->m_entry = NULL;
        query_ex->m_task = task;
        TAILQ_INSERT_TAIL(&query_ex->m_task->m_querys, query_ex, m_next);
    }
    else {
        query_ex->m_entry = entry;
        TAILQ_INSERT_TAIL(&query_ex->m_manage->m_to_notify_querys, query_ex, m_next);
        net_dns_manage_active_delay_process(manage);
    }
    
    return 0;

START_ERROR:
    if (is_entry_new) {
        assert(entry);
        net_dns_entry_free(entry);
    }
    else if (is_task_new) {
        assert(task);
        net_dns_task_free(task);
    }

    return -1;
}

static int net_dns_query_ex_init_for_rdns(
    net_dns_manage_t manage, struct net_dns_query_ex * query_ex,
    net_address_t address, net_dns_query_type_t query_type, const char * policy)
{
    assert(net_address_type(address) == net_address_ipv4 || net_address_type(address) == net_address_ipv6);
    assert(query_type == net_dns_query_domain);

    query_ex->m_manage = manage;
    query_ex->m_task = NULL;
    query_ex->m_query_type = query_type;
    query_ex->m_address = net_address_copy(manage->m_schedule, address);
    if (query_ex->m_address == NULL) {
        CPE_ERROR(
            manage->m_em, "dns-cli: query %s: dup address fail!",
            net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        return -1;
    }

    net_dns_entry_item_t item = net_dns_entry_item_find_by_ip(manage, address);
    if (item != NULL) {
        assert(query_ex->m_address);
        TAILQ_INSERT_TAIL(&manage->m_to_notify_querys, query_ex, m_next);
        net_dns_manage_active_delay_process(manage);
        return 0;
    }

    query_ex->m_task = net_dns_task_create_for_address(manage, address, query_type);
    if (query_ex->m_task == NULL) {
        CPE_ERROR(
            manage->m_em, "dns-cli: query %s: create task fail!",
            net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        net_address_free(query_ex->m_address);
        return -1;
    }

    if (net_dns_task_builder_build(manage->m_builder_default, query_ex->m_task, policy) != 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: query %s: build task fail!",
            net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        net_dns_task_free(query_ex->m_task);
        net_address_free(query_ex->m_address);
        return -1;
    }

    if (net_dns_task_start(query_ex->m_task) != 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: query %s: start task fail!",
            net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        net_dns_task_free(query_ex->m_task);
        net_address_free(query_ex->m_address);
        return -1;
    }
    
    return 0;
}

int net_dns_query_ex_init(
    void * ctx, net_dns_query_t query, net_address_t address, net_dns_query_type_t query_type, const char * policy) {
    net_dns_manage_t manage = ctx;
    struct net_dns_query_ex * query_ex = net_dns_query_data(query);

    if (query_type == net_dns_query_domain) {
        return net_dns_query_ex_init_for_rdns(manage, query_ex, address, query_type, policy);
    }
    else {
        return net_dns_query_ex_init_for_dns(manage, query_ex, address, query_type, policy);
    }
}

void net_dns_query_ex_fini(void * ctx, net_dns_query_t query) {
    struct net_dns_query_ex * query_ex = net_dns_query_data(query);

    if (query_ex->m_query_type == net_dns_query_domain) {
        if (query_ex->m_address) {
            net_address_free(query_ex->m_address);
            query_ex->m_address = NULL;
        }
    }
    
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
