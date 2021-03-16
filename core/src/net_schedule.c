#include "assert.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule_i.h"
#include "net_local_ip_stack_monitor_i.h"
#include "net_driver_i.h"
#include "net_protocol_i.h"
#include "net_endpoint_i.h"
#include "net_watcher_i.h"
#include "net_endpoint_monitor_i.h"
#include "net_endpoint_next_i.h"
#include "net_address_i.h"
#include "net_address_matcher_i.h"
#include "net_address_rule_i.h"
#include "net_timer_i.h"
#include "net_dns_query_i.h"
#include "net_debug_setup_i.h"
#include "net_debug_condition_i.h"
#include "net_mem_group_i.h"
#include "net_mem_block_i.h"
#include "net_mem_group_type_i.h"
#include "net_mem_group_type_basic_i.h"
#include "net_mem_group_type_cache_i.h"
#include "net_mem_group_type_ringbuffer_i.h"
#include "net_protocol_noop.h"
#include "net_protocol_null.h"
#include "net_pair_i.h"

static void net_schedule_do_delay_process(net_timer_t timer, void * input_ctx);

net_schedule_t
net_schedule_create(mem_allocrator_t alloc, error_monitor_t em, net_mem_policy_t mem_policy) {
    net_schedule_t schedule;
    
    schedule = mem_alloc(alloc, sizeof(struct net_schedule));
    if (schedule == NULL) {
        CPE_ERROR(em, "schedule: alloc fail!");
        return NULL;
    }
    bzero(schedule, sizeof(*schedule));

    schedule->m_alloc = alloc;
    schedule->m_em = em;
    schedule->m_debug = 0;
    schedule->m_delay_processor = NULL;
    schedule->m_dns_resolver_ctx = NULL;
    schedule->m_dns_resolver_ctx_fini_fun = NULL;
    schedule->m_dns_query_capacity = 0;
    schedule->m_dns_query_init_fun = NULL;
    schedule->m_dns_query_fini_fun = NULL;
    schedule->m_dns_max_query_id = 0;
    schedule->m_pair_driver = NULL;
    schedule->m_noop_protocol = NULL;
    schedule->m_null_protocol = NULL;
    schedule->m_dgram_max_id = 0;
    schedule->m_endpoint_max_id = 0;
    schedule->m_endpoint_protocol_capacity = 0;
    schedule->m_endpoint_driver_capacity = 0;
    schedule->m_local_ip_stack = net_local_ip_stack_ipv4;
    schedule->m_domain_address_rule = NULL;

    TAILQ_INIT(&schedule->m_local_ip_stack_monitors);
    TAILQ_INIT(&schedule->m_debug_setups);
    TAILQ_INIT(&schedule->m_drivers);
    TAILQ_INIT(&schedule->m_protocols);
    TAILQ_INIT(&schedule->m_mem_group_types);
    TAILQ_INIT(&schedule->m_free_local_ip_stack_monitors);
    TAILQ_INIT(&schedule->m_free_addresses);
    TAILQ_INIT(&schedule->m_free_dns_querys);
    TAILQ_INIT(&schedule->m_free_endpoint_monitors);
    TAILQ_INIT(&schedule->m_free_endpoint_nexts);
    TAILQ_INIT(&schedule->m_free_mem_groups);
    TAILQ_INIT(&schedule->m_free_mem_blocks);

    if (mem_policy == NULL || mem_policy->m_type == net_mem_type_native) {
        schedule->m_dft_mem_group_type = net_mem_group_type_basic_create(schedule);
        if (schedule->m_dft_mem_group_type == NULL) {
            CPE_ERROR(em, "schedule: create default mem group type basic fail");
            goto INIT_FAILED;
        }
    } else if (mem_policy->m_type == net_mem_type_cache) {
        schedule->m_dft_mem_group_type = net_mem_group_type_cache_create(schedule);
        if (schedule->m_dft_mem_group_type == NULL) {
            CPE_ERROR(em, "schedule: create default mem group type cache fail");
            goto INIT_FAILED;
        }
    } else if (mem_policy->m_type == net_mem_type_ringbuffer) {
        schedule->m_dft_mem_group_type = net_mem_group_type_cache_create(schedule);
        if (schedule->m_dft_mem_group_type == NULL) {
            CPE_ERROR(em, "schedule: create default mem group type ringbuffer fail");
            goto INIT_FAILED;
        }
    } else {
        CPE_ERROR(em, "schedule: not support mem type %d", mem_policy->m_type);
        goto INIT_FAILED;
    }

    schedule->m_dft_mem_group = net_mem_group_create(schedule->m_dft_mem_group_type);
    if (schedule->m_dft_mem_group == NULL) {
        CPE_ERROR(em, "schedule: create default mem group fail");
        goto INIT_FAILED;
    }
    
    if (cpe_hash_table_init(
            &schedule->m_endpoints,
            alloc,
            (cpe_hash_fun_t) net_endpoint_hash,
            (cpe_hash_eq_t) net_endpoint_eq,
            CPE_HASH_OBJ2ENTRY(net_endpoint, m_hh),
            -1) != 0)
    {
        goto INIT_FAILED;
    }

    if (cpe_hash_table_init(
            &schedule->m_dns_querys,
            schedule->m_alloc,
            (cpe_hash_fun_t) net_dns_query_hash,
            (cpe_hash_eq_t) net_dns_query_eq,
            CPE_HASH_OBJ2ENTRY(net_dns_query, m_hh),
            -1) != 0)
    {
        cpe_hash_table_fini(&schedule->m_endpoints);
        goto INIT_FAILED;
    }

    mem_buffer_init(&schedule->m_tmp_buffer, alloc);

    /*初始化完成 */
    schedule->m_noop_protocol = net_protocol_noop_create(schedule);
    schedule->m_null_protocol = net_protocol_null_create(schedule);
    schedule->m_pair_driver = net_pair_driver_create(schedule);

    return schedule;

INIT_FAILED:
    assert(schedule->m_pair_driver == NULL);
    assert(schedule->m_null_protocol == NULL);
    assert(schedule->m_noop_protocol == NULL);

    if (schedule->m_dft_mem_group) {
        net_mem_group_free(schedule->m_dft_mem_group);
        schedule->m_dft_mem_group = NULL;
    }

    if (schedule->m_dft_mem_group_type) {
        net_mem_group_type_free(schedule->m_dft_mem_group_type);
        schedule->m_dft_mem_group_type = NULL;
    }

    mem_free(alloc, schedule);
    return NULL;
}

void net_schedule_free(net_schedule_t schedule) {
    net_dns_query_free_all(schedule);
    cpe_hash_table_fini(&schedule->m_dns_querys);

    if (schedule->m_delay_processor != NULL) {
        net_timer_free(schedule->m_delay_processor);
        schedule->m_delay_processor = NULL;
    }
    
    if (schedule->m_dns_resolver_ctx && schedule->m_dns_resolver_ctx_fini_fun) {
        schedule->m_dns_resolver_ctx_fini_fun(schedule->m_dns_resolver_ctx);
        schedule->m_dns_resolver_ctx = NULL;
    }

    while(!TAILQ_EMPTY(&schedule->m_local_ip_stack_monitors)) {
        net_local_ip_stack_monitor_free(TAILQ_FIRST(&schedule->m_local_ip_stack_monitors));
    }
    
    if (schedule->m_pair_driver) {
        net_driver_free(schedule->m_pair_driver);
        schedule->m_pair_driver = NULL;
    }
    
    while(!TAILQ_EMPTY(&schedule->m_drivers)) {
        net_driver_free(TAILQ_LAST(&schedule->m_drivers, net_driver_list));
    }

    if (schedule->m_noop_protocol) {
        net_protocol_free(schedule->m_noop_protocol);
        schedule->m_noop_protocol = NULL;
    }

    if (schedule->m_null_protocol) {
        net_protocol_free(schedule->m_null_protocol);
        schedule->m_null_protocol = NULL;
    }
    
    while(!TAILQ_EMPTY(&schedule->m_protocols)) {
        net_protocol_free(TAILQ_FIRST(&schedule->m_protocols));
    }

    while(!TAILQ_EMPTY(&schedule->m_mem_group_types)) {
        net_mem_group_type_free(TAILQ_FIRST(&schedule->m_mem_group_types));
    }
    assert(schedule->m_dft_mem_group == NULL);
    assert(schedule->m_dft_mem_group_type == NULL);
    
    while(!TAILQ_EMPTY(&schedule->m_debug_setups)) {
        net_debug_setup_free(TAILQ_FIRST(&schedule->m_debug_setups));
    }

    if (schedule->m_domain_address_rule) {
        net_address_rule_free(schedule, schedule->m_domain_address_rule);
        schedule->m_domain_address_rule = NULL;
    }

    while(!TAILQ_EMPTY(&schedule->m_free_local_ip_stack_monitors)) {
        net_local_ip_stack_monitor_real_free(TAILQ_FIRST(&schedule->m_free_local_ip_stack_monitors));
    }

    while(!TAILQ_EMPTY(&schedule->m_free_addresses)) {
        net_address_real_free(TAILQ_FIRST(&schedule->m_free_addresses));
    }
    
    while(!TAILQ_EMPTY(&schedule->m_free_dns_querys)) {
        net_dns_query_real_free(TAILQ_FIRST(&schedule->m_free_dns_querys));
    }
    
    while(!TAILQ_EMPTY(&schedule->m_free_endpoint_monitors)) {
        net_endpoint_monitor_real_free(TAILQ_FIRST(&schedule->m_free_endpoint_monitors));
    }

    while(!TAILQ_EMPTY(&schedule->m_free_endpoint_nexts)) {
        net_endpoint_next_real_free(TAILQ_FIRST(&schedule->m_free_endpoint_nexts));
    }
    
    while(!TAILQ_EMPTY(&schedule->m_free_mem_groups)) {
        net_mem_group_real_free(TAILQ_FIRST(&schedule->m_free_mem_groups));
    }

    while(!TAILQ_EMPTY(&schedule->m_free_mem_blocks)) {
        net_mem_block_real_free(TAILQ_FIRST(&schedule->m_free_mem_blocks));
    }
    
    assert(cpe_hash_table_count(&schedule->m_endpoints) == 0);
    cpe_hash_table_fini(&schedule->m_endpoints);

    mem_buffer_clear(&schedule->m_tmp_buffer);
    mem_free(schedule->m_alloc, schedule);
}

mem_allocrator_t net_schedule_allocrator(net_schedule_t schedule) {
    return schedule->m_alloc;
}

error_monitor_t net_schedule_em(net_schedule_t schedule) {
    return schedule->m_em;
}

net_mem_group_t net_schedule_dft_mem_group(net_schedule_t schedule) {
    return schedule->m_dft_mem_group;
}

mem_buffer_t net_schedule_tmp_buffer(net_schedule_t schedule) {
    return &schedule->m_tmp_buffer;
}

net_protocol_t net_schedule_noop_protocol(net_schedule_t schedule) {
    return schedule->m_noop_protocol;
}

net_protocol_t net_schedule_null_protocol(net_schedule_t schedule) {
    return schedule->m_null_protocol;
}

uint32_t net_schedule_next_endpoint_id(net_schedule_t schedule) {
    return schedule->m_endpoint_max_id + 1;
}

uint8_t net_schedule_debug(net_schedule_t schedule) {
    return schedule->m_debug;
}

void net_schedule_set_debug(net_schedule_t schedule, uint8_t debug) {
    schedule->m_debug = debug;
}

void net_schedule_set_dns_resolver(
    net_schedule_t schedule,
    void * ctx,
    void (*ctx_fini)(void * ctx),
    net_schedule_dns_local_query_fun_t local_query,
    uint16_t dns_query_capacity,
    net_schedule_dns_query_init_fun_t init_fun,
    net_schedule_dns_query_fini_fun_t fini_fun)
{
    schedule->m_dns_resolver_ctx = ctx;
    schedule->m_dns_resolver_ctx_fini_fun = ctx_fini;
    schedule->m_dns_local_query = local_query;
    schedule->m_dns_query_capacity = dns_query_capacity;
    schedule->m_dns_query_init_fun = init_fun;
    schedule->m_dns_query_fini_fun = fini_fun;
}

void * net_schedule_dns_resolver(net_schedule_t schedule) {
    return schedule->m_dns_resolver_ctx;
}

int net_schedule_dns_local_query(
    net_schedule_t schedule,
    net_address_t hostname, net_address_it_t resolved_it, uint8_t recursive)
{
    if (schedule->m_dns_local_query == NULL) {
        CPE_ERROR(schedule->m_em, "core: dns: not support local query!");
        return -1;
    }

    return schedule->m_dns_local_query(schedule->m_dns_resolver_ctx, hostname, resolved_it, recursive);
}

int net_debug_local_host(net_schedule_t schedule, const char * local, uint8_t protocol_debug, uint8_t driver_debug) {
    net_address_t local_address = net_address_create_auto(schedule, local);
    if (local_address == NULL) {
        CPE_ERROR(schedule->m_em, "core: debug: host %s format error!", local);
        return -1;
    }
    
    net_debug_setup_t setup = net_debug_setup_create(schedule, protocol_debug, driver_debug);
    if (setup == NULL) {
        CPE_ERROR(schedule->m_em, "core: debug: create setup for host %s fail!", local);
        net_address_free(local_address);
        return -1;
    }

    if (net_debug_condition_address_create(setup, net_debug_condition_scope_local, local_address) == NULL) {
        net_address_free(local_address);
        return -1;
    }

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "core: debug: local %s -> protocol=%d, driver=%d!", local, protocol_debug, driver_debug);
    }
    
    return 0;
}

int net_debug_remote_host(net_schedule_t schedule, const char * remote, uint8_t protocol_debug, uint8_t driver_debug) {
    net_address_t remote_address = net_address_create_auto(schedule, remote);
    if (remote_address == NULL) {
        CPE_ERROR(schedule->m_em, "core: debug: host %s format error!", remote);
        return -1;
    }
    
    net_debug_setup_t setup = net_debug_setup_create(schedule, protocol_debug, driver_debug);
    if (setup == NULL) {
        CPE_ERROR(schedule->m_em, "core: debug: create setup for host %s fail!", remote);
        net_address_free(remote_address);
        return -1;
    }

    if (net_debug_condition_address_create(setup, net_debug_condition_scope_remote, remote_address) == NULL) {
        net_address_free(remote_address);
        return -1;
    }
    net_address_free(remote_address);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "core: debug: remote %s -> protocol=%d, driver=%d!", remote, protocol_debug, driver_debug);
    }
    
    return 0;
}

int net_debug_protocol(net_schedule_t schedule, const char * protocol_name, uint8_t debug) {
    net_protocol_t protocol = net_protocol_find(schedule, protocol_name);
    if (protocol == NULL) {
        CPE_ERROR(schedule->m_em, "core: debug: protocol %s not exist!", protocol_name);
        return -1;
    }

    protocol->m_debug = debug;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "core: debug: protocol %s -> %d", protocol_name, debug);
    }
    
    return 0;
}

int net_debug_driver(net_schedule_t schedule, const char * driver_name, uint8_t debug) {
    net_driver_t driver = net_driver_find(schedule, driver_name);
    if (driver == NULL) {
        CPE_ERROR(schedule->m_em, "core: debug: driver %s not exist!", driver_name);
        return -1;
    }

    driver->m_debug = debug;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "core: debug: driver %s -> %d", driver_name, debug);
    }
    
    return 0;
}

void net_schedule_start_delay_process(net_schedule_t schedule) {
    if (schedule->m_delay_processor == NULL) {
        schedule->m_delay_processor = net_timer_auto_create(schedule, net_schedule_do_delay_process, schedule);
        if (schedule->m_delay_processor == NULL) {
            CPE_ERROR(schedule->m_em, "schedule: create delay processor fail!");
            return;
        }
    }

    if (!net_timer_is_active(schedule->m_delay_processor)) {
        net_timer_active(schedule->m_delay_processor, 0);
    }
}

static void net_schedule_do_delay_process(net_timer_t timer, void * input_ctx) {
    net_schedule_t schedule = input_ctx;
    net_driver_t driver;

    TAILQ_FOREACH(driver, &schedule->m_drivers, m_next_for_schedule) {
        while(!TAILQ_EMPTY(&driver->m_deleting_endpoints)) {
            net_endpoint_free(TAILQ_FIRST(&driver->m_deleting_endpoints));
        }
    }
}

const char * net_mem_type_str(net_mem_type_t mem_type) {
    switch(mem_type) {
    case net_mem_type_native:
        return "native";
    case net_mem_type_cache:
        return "cache";
    case net_mem_type_ringbuffer:
        return "ringbuffer";
    }
}
