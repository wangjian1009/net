#ifndef NET_LOG_QUEUE_I_H_INCLEDED
#define NET_LOG_QUEUE_I_H_INCLEDED
#include "net_log_schedule_i.h"

net_log_queue_t net_log_queue_create(int32_t max_size);
void net_log_queue_free(net_log_queue_t queue);

int32_t net_log_queue_size(net_log_queue_t queue);
int32_t net_log_queue_isfull(net_log_queue_t queue);

int32_t net_log_queue_push(net_log_queue_t queue, void * data);
void * net_log_queue_pop(net_log_queue_t queue);

#endif
