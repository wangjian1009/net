#ifndef NET_MEM_GROUP_H_INCLEDED
#define NET_MEM_GROUP_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_mem_group_t net_mem_group_create(net_schedule_t schedule, uint32_t capacity);
void net_mem_group_free(net_mem_group_t mem_group);

NET_END_DECL

#endif
