#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "log_queue.h"

struct _log_queue{
    void ** data;
    int64_t head;
    int64_t tail;
    int32_t size;
};

log_queue * log_queue_create(int32_t size) {
    void * buffer = malloc(sizeof(void *) * size + sizeof(log_queue));
    memset(buffer, 0, sizeof(void *) * size + sizeof(log_queue));
    log_queue * queue = (log_queue *)buffer;
    queue->data = (void **)(buffer + sizeof(log_queue));
    queue->size = size;
    return queue;
}

void log_queue_destroy(log_queue * queue) {
    free(queue);
}

int32_t log_queue_size(log_queue * queue) {
    return (int32_t)(queue->tail - queue->head);
}

int32_t log_queue_isfull(log_queue * queue) {
    int32_t rst = (int32_t)((queue->tail - queue->head) == queue->size);
    return rst;
}

int32_t log_queue_push(log_queue * queue, void * data) {
    if (queue->tail - queue->head == queue->size)
    {
        return -1;
    }
    queue->data[queue->tail++ % queue->size] = data;
    return 0;
}

void * log_queue_trypop(log_queue * queue) {
    void * result = NULL;
    if (queue->tail > queue->head) {
        result = queue->data[queue->head++ % queue->size];
    }
    return result;
}

