#ifndef NET_MEM_GROUP_TYPE_H_INCLEDED
#define NET_MEM_GROUP_TYPE_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_mem_alloc_capacity_policy {
    net_mem_alloc_capacity_at_least,
    net_mem_alloc_capacity_suggest,
} net_mem_alloc_capacity_policy_t;

typedef int (*net_mem_group_type_init_fun_t)(net_mem_group_type_t type);
typedef void (*net_mem_group_type_fini_fun_t)(net_mem_group_type_t type);
typedef uint32_t (*net_mem_gruop_type_suggest_size_fun_t)(net_mem_group_type_t type);

typedef void * (*net_mem_block_alloc_fun_t)(net_mem_group_type_t type, uint32_t * capacity, net_mem_alloc_capacity_policy_t policy);
typedef void (*net_mem_block_free_fun_t)(net_mem_group_type_t type, void * data, uint32_t capacity);

net_mem_group_type_t
net_mem_group_type_create(
    net_schedule_t schedule, net_mem_type_t type,
    uint32_t capacity,
    net_mem_group_type_init_fun_t init_fun,
    net_mem_group_type_fini_fun_t fini_fun,
    net_mem_gruop_type_suggest_size_fun_t suggest_size,
    /*block*/
    net_mem_block_alloc_fun_t block_alloc,
    net_mem_block_free_fun_t block_free);

void net_mem_group_type_free(net_mem_group_type_t mem_group);

net_mem_group_type_t
net_mem_group_type_find(net_schedule_t schedule, const char * name);

const char * net_mem_group_type_name(net_mem_group_type_t type);

NET_END_DECL

#endif
