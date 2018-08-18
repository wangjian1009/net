#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol_i.h"

net_protocol_t
net_protocol_create(
    net_schedule_t schedule,
    const char * name,
    /*protocol*/
    uint16_t protocol_capacity,
    net_protocol_init_fun_t protocol_init,
    net_protocol_fini_fun_t protocol_fini,
    /*endpoint*/
    uint16_t endpoint_capacity,
    net_protocol_endpoint_init_fun_t endpoint_init,
    net_protocol_endpoint_fini_fun_t endpoint_fini,
    net_protocol_endpoint_input_fun_t endpoint_input,
    net_protocol_endpoint_forward_fun_t endpoint_forward,
    net_protocol_endpoint_direct_fun_t endpoint_direct,
    net_protocol_endpoint_on_state_change_fun_t endpoint_on_state_chagne)
{
    net_protocol_t protocol;

    if (schedule->m_endpoint_max_id != 0) {
        CPE_ERROR(schedule->m_em, "protocol: already have endpoint created!");
        return NULL;
    }
    
    protocol = mem_alloc(schedule->m_alloc, sizeof(struct net_protocol) + protocol_capacity);
    if (protocol == NULL) {
        CPE_ERROR(schedule->m_em, "protocol: alloc fail");
        return NULL;
    }

    protocol->m_schedule = schedule;
    cpe_str_dup(protocol->m_name, sizeof(protocol->m_name), name);
    protocol->m_debug = 0;

    protocol->m_protocol_capacity = protocol_capacity;
    protocol->m_protocol_init = protocol_init;
    protocol->m_protocol_fini = protocol_fini;

    protocol->m_endpoint_capacity = endpoint_capacity;
    protocol->m_endpoint_init = endpoint_init;
    protocol->m_endpoint_fini = endpoint_fini;
    protocol->m_endpoint_input = endpoint_input;
    protocol->m_endpoint_forward = endpoint_forward;
    protocol->m_endpoint_direct = endpoint_direct;
    protocol->m_endpoint_on_state_chagne = endpoint_on_state_chagne;

    if (protocol->m_protocol_init(protocol) != 0) {
        mem_free(schedule->m_alloc, protocol);
        return NULL;
    }

    if (endpoint_capacity > schedule->m_endpoint_protocol_capacity) {
        schedule->m_endpoint_protocol_capacity = endpoint_capacity;
    }
    
    TAILQ_INSERT_TAIL(&schedule->m_protocols, protocol, m_next_for_schedule);
    
    return protocol;
}

void net_protocol_free(net_protocol_t protocol) {
    net_schedule_t schedule = protocol->m_schedule;

    protocol->m_protocol_fini(protocol);
    
    TAILQ_REMOVE(&schedule->m_protocols, protocol, m_next_for_schedule);

    mem_free(schedule->m_alloc, protocol);
}

net_protocol_t net_protocol_find(net_schedule_t schedule, const char * name) {
    net_protocol_t protocol;

    TAILQ_FOREACH(protocol, &schedule->m_protocols, m_next_for_schedule) {
        if (strcmp(protocol->m_name, name) == 0) return protocol;
    }
    
    return NULL;
}

net_schedule_t net_protocol_schedule(net_protocol_t protocol) {
    return protocol->m_schedule;
}

const char * net_protocol_name(net_protocol_t protocol) {
    return protocol->m_name;
}

uint8_t net_protocol_debug(net_protocol_t protocol) {
    return protocol->m_debug;
}

void net_protocol_set_debug(net_protocol_t protocol, uint8_t debug) {
    protocol->m_debug = debug;
}

uint8_t net_protocol_support_direct(net_protocol_t protocol) {
    return protocol->m_endpoint_direct == NULL ? 0 : 1;
}

void * net_protocol_data(net_protocol_t protocol) {
    return protocol + 1;
}

net_protocol_t net_protocol_from_data(void * data) {
    return  ((net_protocol_t)data) - 1;
}

