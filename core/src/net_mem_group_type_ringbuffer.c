#include <assert.h>
#include "net_mem_group_type_ringbuffer_i.h"
#include "net_mem_group_type_i.h"

    /* schedule->m_endpoint_buf = ringbuffer_new(common_buff_capacity, em); */
    /* if (schedule->m_endpoint_buf == NULL) { */
    /*     CPE_ERROR(em, "schedule: alloc common buff fail, capacity=%d!", common_buff_capacity); */
    /*     return NULL; */
    /* } */
    /* schedule->m_endpoint_tb = NULL; */


int net_mem_group_type_ringbuffer_init(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t ringbuffer = net_mem_group_type_data(type);

    ringbuffer->m_alloced_count = 0;
    ringbuffer->m_alloced_size = 0;
    ringbuffer->m_group_count = 0;

    uint32_t common_buff_capacity = 5000;
    ringbuffer->m_ring_buffer = ringbuffer_new(common_buff_capacity, type->m_schedule->m_em);
    if (ringbuffer->m_ring_buffer == NULL) {
        CPE_ERROR(schedule->m_em, "schedule: alloc common buff fail, capacity=%d!", common_buff_capacity);
        return -1;
    }

    return 0;
}

void net_mem_group_type_ringbuffer_fini(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t ringbuffer = net_mem_group_type_data(type);
}

uint32_t net_mem_group_type_ringbuffer_suggest_size(net_mem_group_type_t type) {
    return 0;
}

void * net_mem_group_type_ringbuffer_alloc(
    net_mem_group_type_t type, uint32_t * capacity, net_mem_alloc_capacity_policy_t policy)
{
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t ringbuffer = net_mem_group_type_data(type);

    //TODO:
    return NULL;
}

void net_mem_group_type_ringbuffer_free(net_mem_group_type_t type, void * data, uint32_t capacity) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t ringbuffer = net_mem_group_type_data(type);
}

net_mem_group_type_t
net_mem_group_type_ringbuffer_create(net_schedule_t schedule) {
    return net_mem_group_type_create(
        schedule, "ring-buffer",
        sizeof(struct net_mem_group_type_ringbuffer),
        net_mem_group_type_ringbuffer_init, net_mem_group_type_ringbuffer_fini,
        net_mem_group_type_ringbuffer_suggest_size,
        net_mem_group_type_ringbuffer_alloc, net_mem_group_type_ringbuffer_free);
}
