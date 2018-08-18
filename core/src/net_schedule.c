#include "assert.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/ringbuffer.h"
#include "net_schedule_i.h"
#include "net_driver_i.h"
#include "net_protocol_i.h"
#include "net_endpoint_i.h"
#include "net_endpoint_monitor_i.h"
#include "net_address_i.h"
#include "net_address_matcher_i.h"
#include "net_timer_i.h"
#include "net_link_i.h"
#include "net_direct_endpoint_i.h"
#include "net_dns_query_i.h"
#include "net_debug_setup_i.h"

static void net_schedule_do_delay_process(net_timer_t timer, void * input_ctx);

net_schedule_t
net_schedule_create(mem_allocrator_t alloc, error_monitor_t em, uint32_t common_buff_capacity) {
    net_schedule_t schedule;
    
    schedule = mem_alloc(alloc, sizeof(struct net_schedule));
    if (schedule == NULL) {
        CPE_ERROR(em, "schedule: alloc fail!");
        return NULL;
    }

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
    schedule->m_direct_protocol = NULL;
    schedule->m_direct_driver = NULL;
    schedule->m_endpoint_max_id = 0;
    schedule->m_endpoint_protocol_capacity = 0;

    TAILQ_INIT(&schedule->m_debug_setups);
    TAILQ_INIT(&schedule->m_drivers);
    TAILQ_INIT(&schedule->m_protocols);
    TAILQ_INIT(&schedule->m_links);
    TAILQ_INIT(&schedule->m_free_addresses);
    TAILQ_INIT(&schedule->m_free_links);
    TAILQ_INIT(&schedule->m_free_dns_querys);
    TAILQ_INIT(&schedule->m_free_endpoint_monitors);

    schedule->m_endpoint_buf = ringbuffer_new(common_buff_capacity);
    if (schedule->m_endpoint_buf == NULL) {
        CPE_ERROR(em, "schedule: alloc common buff fail, capacity=%d!", common_buff_capacity);
        return NULL;
    }
    schedule->m_endpoint_tb = NULL;

    if (cpe_hash_table_init(
            &schedule->m_endpoints,
            alloc,
            (cpe_hash_fun_t) net_endpoint_hash,
            (cpe_hash_eq_t) net_endpoint_eq,
            CPE_HASH_OBJ2ENTRY(net_endpoint, m_hh),
            -1) != 0)
    {
        ringbuffer_delete(schedule->m_endpoint_buf);
        mem_free(alloc, schedule);
        return NULL;
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
        ringbuffer_delete(schedule->m_endpoint_buf);
        mem_free(alloc, schedule);
        return NULL;
    }

    mem_buffer_init(&schedule->m_tmp_buffer, alloc);

    schedule->m_direct_protocol = net_direct_protocol_create(schedule);
    if (schedule->m_direct_protocol == NULL) {
        net_schedule_free(schedule);
        return NULL;
    }
    
    return schedule;
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

    while(!TAILQ_EMPTY(&schedule->m_links)) {
        net_link_free(TAILQ_FIRST(&schedule->m_links));
    }

    while(!TAILQ_EMPTY(&schedule->m_drivers)) {
        net_driver_free(TAILQ_FIRST(&schedule->m_drivers));
    }

    if (schedule->m_direct_protocol) {
        net_protocol_free(schedule->m_direct_protocol);
        schedule->m_direct_protocol = NULL;
    }

    while(!TAILQ_EMPTY(&schedule->m_protocols)) {
        net_protocol_free(TAILQ_FIRST(&schedule->m_protocols));
    }

    while(!TAILQ_EMPTY(&schedule->m_debug_setups)) {
        net_debug_setup_free(TAILQ_FIRST(&schedule->m_debug_setups));
    }

    while(!TAILQ_EMPTY(&schedule->m_free_addresses)) {
        net_address_real_free(TAILQ_FIRST(&schedule->m_free_addresses));
    }

    while(!TAILQ_EMPTY(&schedule->m_free_links)) {
        net_link_real_free(TAILQ_FIRST(&schedule->m_free_links));
    }

    while(!TAILQ_EMPTY(&schedule->m_free_dns_querys)) {
        net_dns_query_real_free(TAILQ_FIRST(&schedule->m_free_dns_querys));
    }
    
    while(!TAILQ_EMPTY(&schedule->m_free_endpoint_monitors)) {
        net_endpoint_monitor_real_free(TAILQ_FIRST(&schedule->m_free_endpoint_monitors));
    }
    
    assert(cpe_hash_table_count(&schedule->m_endpoints) == 0);
    cpe_hash_table_fini(&schedule->m_endpoints);

    ringbuffer_delete(schedule->m_endpoint_buf);
    schedule->m_endpoint_buf = NULL;
    
    mem_buffer_clear(&schedule->m_tmp_buffer);
    mem_free(schedule->m_alloc, schedule);
}

mem_allocrator_t net_schedule_allocrator(net_schedule_t schedule) {
    return schedule->m_alloc;
}

error_monitor_t net_schedule_em(net_schedule_t schedule) {
    return schedule->m_em;
}

mem_buffer_t net_schedule_tmp_buffer(net_schedule_t schedule) {
    return &schedule->m_tmp_buffer;
}

net_protocol_t net_schedule_direct_protocol(net_schedule_t schedule) {
    return schedule->m_direct_protocol;
}

net_driver_t net_schedule_direct_driver(net_schedule_t schedule) {
    return schedule->m_direct_driver;
}

void net_schedule_set_direct_driver(net_schedule_t schedule, net_driver_t driver) {
    schedule->m_direct_driver = driver;
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
    uint16_t dns_query_capacity,
    net_schedule_dns_query_init_fun_t init_fun,
    net_schedule_dns_query_fini_fun_t fini_fun)
{
    schedule->m_dns_resolver_ctx = ctx;
    schedule->m_dns_resolver_ctx_fini_fun = ctx_fini;
    schedule->m_dns_query_capacity = dns_query_capacity;
    schedule->m_dns_query_init_fun = init_fun;
    schedule->m_dns_query_fini_fun = fini_fun;
}

void * net_schedule_dns_resolver(net_schedule_t schedule) {
    return schedule->m_dns_resolver_ctx;
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
