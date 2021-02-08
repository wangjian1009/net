#include "net_mem_group_type_basic_i.h"
#include "net_mem_group_type_i.h"

void * net_mem_group_type_basic_alloc(net_mem_group_type_t type, uint32_t * capacity) {
    if (*capacity == 0) *capacity = 16;
    return mem_alloc(type->m_schedule->m_alloc, *capacity);
}

void net_mem_group_type_basic_free(net_mem_group_type_t type, void * data, uint32_t capacity) {
    mem_free(type->m_schedule->m_alloc, data);
}

net_mem_group_type_t
net_mem_group_type_basic_create(net_schedule_t schedule) {
    return net_mem_group_type_create(
        schedule, "basic",
        0, NULL, NULL,
        net_mem_group_type_basic_alloc, net_mem_group_type_basic_free);
}
