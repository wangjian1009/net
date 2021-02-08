#ifndef NET_MEM_GROUP_TYPE_H_INCLEDED
#define NET_MEM_GROUP_TYPE_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_mem_group_type_t
net_mem_group_type_create(
    net_schedule_t schedule, const char * name, uint32_t capacity);

void net_mem_group_type_free(net_mem_group_type_t mem_group);

net_mem_group_type_t
net_mem_group_type_find(net_schedule_t schedule, const char * name);

const char * net_mem_group_type_name(net_mem_group_type_t type);

NET_END_DECL

#endif
