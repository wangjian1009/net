#ifndef NET_MEM_GROUP_H_INCLEDED
#define NET_MEM_GROUP_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_mem_group_t net_mem_group_create(net_mem_group_type_t type);
void net_mem_group_free(net_mem_group_t mem_group);

net_mem_group_type_t net_mem_group_type(net_mem_group_t mem_group);
uint32_t net_mem_group_alloced_count(net_mem_group_t mem_group);
uint64_t net_mem_group_alloced_size(net_mem_group_t mem_group);

uint32_t net_mem_group_suggest_size(net_mem_group_t mem_group);

NET_END_DECL

#endif
