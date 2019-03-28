#include "net_log_flusher.h"
#include "log_queue.h"

static void * net_log_flusher_thread(void * param);

net_log_flusher_t
net_log_flusher_create(net_log_category_t category) {
    net_log_schedule_t schedule = category->m_schedule;

    net_log_flusher_t flusher = mem_alloc(schedule->m_alloc, sizeof(struct net_log_flusher));
    if (flusher == NULL) {
        CPE_ERROR(schedule->m_em, "log: category [%d]%s: flusher: alloc fail", category->m_id, category->m_name);
        return NULL;
    }

    flusher->m_category = category;
    flusher->m_queue = log_queue_create(32);
    if (flusher->m_queue == NULL) {
        CPE_ERROR(schedule->m_em, "log: category [%d]%s: flusher: queue create fail", category->m_id, category->m_name);
        mem_free(schedule->m_alloc, flusher);
        return NULL;
    }
    
    return flusher;
}

void net_log_flusher_free(net_log_flusher_t flusher);

static void * net_log_flusher_thread(void * param) {
    net_log_flusher_t flusher = param;
    net_log_category_t category = flusher->m_category;
    net_log_schedule_t schedule = category->m_schedule;

    return NULL;
}
