#ifndef NET_MEM_BLOCK_H_INCLEDED
#define NET_MEM_BLOCK_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_mem_block_t net_mem_block_create(net_mem_group_t group, uint32_t capacity);
net_mem_block_t net_mem_block_create_from(net_mem_group_t group, const uint8_t *data, uint32_t len);
void net_mem_block_free(net_mem_block_t mem_bloc);

uint32_t net_mem_block_get_size(net_mem_block_t ptr);
uint8_t * net_mem_block_data(net_mem_block_t ptr);

int net_mem_block_compare(net_mem_block_t ptr1, net_mem_block_t ptr2, uint32_t size);
void net_mem_block_reset(net_mem_block_t ptr);
net_mem_block_t net_mem_block_clone(net_mem_group_t group, net_mem_block_t ptr);
uint32_t net_mem_block_realloc(net_mem_block_t ptr, uint32_t capacity);
void net_mem_block_insert(net_mem_block_t ptr, uint32_t pos, const uint8_t *data, uint32_t size);
void net_mem_block_insert2(net_mem_block_t ptr, uint32_t pos, net_mem_block_t data);
uint32_t net_mem_block_store(net_mem_block_t ptr, const uint8_t *data, uint32_t size);
void net_mem_block_replace(net_mem_block_t dst, net_mem_block_t src);
uint32_t net_mem_block_concatenate(net_mem_block_t ptr, const uint8_t *data, uint32_t size);
uint32_t net_mem_block_concatenate2(net_mem_block_t dst, net_mem_block_t src);
void net_mem_block_shortened_to(net_mem_block_t ptr, uint32_t begin, uint32_t len);

void net_mem_block_set_size(net_mem_block_t block, uint32_t size);
void net_mem_block_erase(net_mem_block_t block, uint32_t size);
void net_mem_block_append(net_mem_block_t block, void const * data, uint32_t size);

NET_END_DECL

#endif
