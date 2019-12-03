#ifndef NET_BUFFER_H_INCLEDED
#define NET_BUFFER_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_buffer_t net_buffer_create(net_schedule_t scheudle, uint32_t capacity);
net_buffer_t net_buffer_create_from(net_schedule_t scheudle, const uint8_t *data, uint32_t len);
uint32_t net_buffer_get_length(net_buffer_t ptr);
const uint8_t * net_buffer_get_data(net_buffer_t ptr, uint32_t *length);
void net_buffer_add_ref(net_buffer_t ptr);
void net_buffer_release(net_buffer_t ptr);
int net_buffer_compare(net_buffer_t ptr1, net_buffer_t ptr2, uint32_t size);
void net_buffer_reset(net_buffer_t ptr);
net_buffer_t net_buffer_clone(net_buffer_t ptr);
uint32_t net_buffer_realloc(net_buffer_t ptr, uint32_t capacity);
void net_buffer_insert(net_buffer_t ptr, uint32_t pos, const uint8_t *data, uint32_t size);
void net_buffer_insert2(net_buffer_t ptr, uint32_t pos, net_buffer_t data);
uint32_t net_buffer_store(net_buffer_t ptr, const uint8_t *data, uint32_t size);
void net_buffer_replace(net_buffer_t dst, net_buffer_t src);
uint32_t net_buffer_concatenate(net_buffer_t ptr, const uint8_t *data, uint32_t size);
uint32_t net_buffer_concatenate2(net_buffer_t dst, net_buffer_t src);
void net_buffer_shortened_to(net_buffer_t ptr, uint32_t begin, uint32_t len);

NET_END_DECL

#endif
