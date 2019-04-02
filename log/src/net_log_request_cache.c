#include "assert.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_string.h"
#include "cpe/vfs/vfs_file.h"
#include "net_log_request_cache.h"
#include "net_log_request.h"
#include "net_log_category_i.h"

net_log_request_cache_t
net_log_request_cache_create(net_log_request_manage_t mgr, uint32_t id, net_log_request_cache_state_t state) {
    net_log_schedule_t schedule = mgr->m_schedule;

    net_log_request_cache_t cache = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_cache));
    if (cache == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: cache %d: alloc fail", mgr->m_name, id);
        return NULL;
    }

    cache->m_mgr = mgr;
    cache->m_id = id;
    cache->m_state = state;
    cache->m_size = 0;
    cache->m_file = NULL;

    if (cache->m_state == net_log_request_cache_building) {
        const char * file = net_log_request_manage_cache_file(mgr, id, mgr->m_tmp_buffer);
        if (file == NULL) {
            CPE_ERROR(schedule->m_em, "log: %s: cache %d: alloc cache file fail", mgr->m_name, id);
            mem_free(schedule->m_alloc, cache);
            return NULL;
        }

        cache->m_file = vfs_file_open(schedule->m_vfs, file, "w");
        if (cache->m_file == NULL) {
            CPE_ERROR(schedule->m_em, "log: %s: cache %d: open file %s fail", mgr->m_name, id, file);
            mem_free(schedule->m_alloc, cache);
            return NULL;
        }
    }
    
    TAILQ_INSERT_HEAD(&mgr->m_caches, cache, m_next);

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: %s: cache %d: created, state=%s",
            mgr->m_name, id, net_log_request_cache_state_str(cache->m_state));
    }
    
    return cache;
}

void net_log_request_cache_free(net_log_request_cache_t cache) {
    net_log_request_manage_t mgr = cache->m_mgr;
    net_log_schedule_t schedule = mgr->m_schedule;

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: %s: cache %d: free, state=%s",
            mgr->m_name, cache->m_id, net_log_request_cache_state_str(cache->m_state));
    }
    
    if (cache->m_file) {
        vfs_file_close(cache->m_file);
        cache->m_file = NULL;
    }

    TAILQ_REMOVE(&mgr->m_caches, cache, m_next);
    
    mem_free(schedule->m_alloc, cache);
}

CPE_START_PACKED

struct CPE_PACKED net_log_request_cache_head {
    uint8_t category_id;
    uint8_t _reserve;
    uint16_t log_count;
    uint32_t magic_num;
    uint32_t builder_time;    
    uint32_t compressed_size;
    uint32_t raw_size;
};

CPE_END_PACKED

int net_log_request_cache_push(net_log_request_cache_t cache, net_log_request_param_t send_param) {
    net_log_request_manage_t mgr = cache->m_mgr;
    net_log_schedule_t schedule = mgr->m_schedule;

    if (cache->m_state != net_log_request_cache_building) {
        CPE_ERROR(
            schedule->m_em, "log: %s: cache %d: push: state is %s, can`t push",
            mgr->m_name, cache->m_id, net_log_request_cache_state_str(cache->m_state));
        return -1;
    }

    net_log_category_t category = send_param->category;

    struct net_log_request_cache_head head;
    head.category_id = category->m_id;
    head._reserve = 0;
    head.log_count = send_param->log_count;
    head.magic_num = send_param->magic_num;
    head.builder_time = send_param->builder_time;
    head.compressed_size = send_param->log_buf->length;
    head.raw_size = send_param->log_buf->raw_length;
    
    assert(cache->m_file);
    if (vfs_file_write(cache->m_file, &head, sizeof(head)) != sizeof(head)) {
        CPE_ERROR(
            schedule->m_em, "log: %s: cache %d: push: write head fail, error=%d (%s)",
            mgr->m_name, cache->m_id, errno, strerror(errno));
        return -1;
    }

    if (vfs_file_write(cache->m_file, send_param->log_buf->data, send_param->log_buf->length) != send_param->log_buf->length) {
        CPE_ERROR(
            schedule->m_em, "log: %s: cache %d: push: write body error, length=%d, error=%d (%s)",
            mgr->m_name, cache->m_id, send_param->log_buf->length, errno, strerror(errno));
        return -1;
    }
    
    cache->m_size += sizeof(head) + send_param->log_buf->length;
    return 0;
}

int net_log_request_cache_cmp(net_log_request_cache_t l, net_log_request_cache_t r, void * ctx) {
    return l->m_id < r->m_id
        ? (- (r->m_id - l->m_id))
        : (l->m_id - r->m_id);
}

const char * net_log_request_cache_state_str(net_log_request_cache_state_t state) {
    switch(state) {
    case net_log_request_cache_building:
        return "building";
    case net_log_request_cache_done:
        return "done";
    }
}
