#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_log_queue.h"

struct net_log_queue{
    void ** data;
    int64_t head;
    int64_t tail;
    int32_t size;
};

net_log_queue_t net_log_queue_create(int32_t size) {
    void * buffer = malloc(sizeof(void *) * size + sizeof(struct net_log_queue));
    memset(buffer, 0, sizeof(void *) * size + sizeof(struct net_log_queue));
    net_log_queue_t queue = (net_log_queue_t)buffer;
    queue->data = (void **)(buffer + sizeof(struct net_log_queue));
    queue->size = size;
    return queue;
}

void net_log_queue_free(net_log_queue_t queue) {
    free(queue);
}

int32_t net_log_queue_size(net_log_queue_t queue) {
    return (int32_t)(queue->tail - queue->head);
}

int32_t net_log_queue_isfull(net_log_queue_t queue) {
    int32_t rst = (int32_t)((queue->tail - queue->head) == queue->size);
    return rst;
}

int32_t net_log_queue_push(net_log_queue_t queue, void * data) {
    if (queue->tail - queue->head == queue->size) {
        return -1;
    }
    
    queue->data[queue->tail++ % queue->size] = data;
    return 0;
}

void * net_log_queue_pop(net_log_queue_t queue) {
    void * result = NULL;
    if (queue->tail > queue->head) {
        result = queue->data[queue->head++ % queue->size];
    }
    return result;
}

