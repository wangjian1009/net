#include <assert.h>
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/tailq_sort.h"
#include "cpe/utils/string_utils.h"
#include "cpe/vfs/vfs_file.h"
#include "cpe/vfs/vfs_dir.h"
#include "cpe/vfs/vfs_entry_info.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_request.h"
#include "net_log_request_cache.h"
#include "net_log_statistic_i.h"

int net_log_thread_search_cache(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;

    const char * cache_dir = net_log_thread_cache_dir(log_thread, &log_thread->m_tmp_buffer);
    if (cache_dir == NULL) {
        CPE_ERROR(schedule->m_em, "log: thread %s: manage: search cache: calc cache dir fail", log_thread->m_name);
        return -1;
    }

    if (!vfs_dir_exist(schedule->m_vfs, cache_dir)) {
        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: thread %s: manage: search cache: cache dir %s not exist, skip",
                log_thread->m_name, cache_dir);
        }
        return 0;
    }
    
    vfs_dir_t d = vfs_dir_open(schedule->m_vfs, cache_dir);
    if (d == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: manage: search cache: cache dir %s open fail, error=%d (%s)",
            log_thread->m_name, cache_dir, errno, strerror(errno));
        return -1;
    }

    struct vfs_entry_info_it entry_it;
    vfs_dir_entries(d, &entry_it);

    int rv = 0;

    vfs_entry_info_t entry;
    while((entry = vfs_entry_info_it_next(&entry_it))) {
        if (entry->m_type != vfs_entry_file) continue;
        if (!cpe_str_start_with(entry->m_name, "cache_")) continue;

        const char * str_id = entry->m_name + 6;
        uint32_t id = atoi(str_id);

        net_log_request_cache_t cache = net_log_request_cache_create(log_thread, id, net_log_request_cache_done, 0);
        if (cache == NULL) {
            CPE_ERROR(
                schedule->m_em, "log: thread %s: manage: search cache: cache %s(id=%d) create fail",
                log_thread->m_name, entry->m_name, id);
            rv = -1;
            continue;
        }

        if (id > log_thread->m_cache_max_id) log_thread->m_cache_max_id = id;
        
        net_log_statistic_cache_created(log_thread);
    }

    vfs_dir_close(d);

    TAILQ_SORT(&log_thread->m_caches, net_log_request_cache, net_log_request_cache_list, m_next, net_log_request_cache_cmp, NULL);

    if (schedule->m_debug) {
        net_log_request_cache_t cache;
        TAILQ_FOREACH(cache, &log_thread->m_caches, m_next) {
            CPE_INFO(schedule->m_em, "log: thread %s: manage: found cache %d", log_thread->m_name, cache->m_id);
        }
    }

    return rv;
}

