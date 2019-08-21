#ifndef NET_LOG_REQUEST_CACHE_H_INCLEDED
#define NET_LOG_REQUEST_CACHE_H_INCLEDED
#include "net_log_thread_i.h"

typedef enum net_log_request_cache_state {
    net_log_request_cache_building,
    net_log_request_cache_done,
} net_log_request_cache_state_t;

struct net_log_request_cache {
    net_log_thread_t m_thread;
    TAILQ_ENTRY(net_log_request_cache) m_next;
    uint32_t m_id;
    net_log_request_cache_state_t m_state;
    uint32_t m_size;
    uint8_t m_is_package_counted;
    vfs_file_t m_file;
};

net_log_request_cache_t
net_log_request_cache_create(
    net_log_thread_t thread, uint32_t id, net_log_request_cache_state_t state, uint8_t is_package_counted);
void net_log_request_cache_free(net_log_request_cache_t cache);

void net_log_request_cache_clear_and_free(net_log_request_cache_t cache);

int net_log_request_cache_append(net_log_request_cache_t cache, net_log_request_param_t send_param);
int net_log_request_cache_load(net_log_request_cache_t cache);
int net_log_request_cache_close(net_log_request_cache_t cache);

int net_log_request_cache_cmp(net_log_request_cache_t l, net_log_request_cache_t r, void * ctx);

const char * net_log_request_cache_state_str(net_log_request_cache_state_t state);

#endif
