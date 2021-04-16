#include <assert.h>
#include "cpe/utils/math_ex.h"
#include "cpe/pal/pal_string.h"
#include "net_driver_i.h"
#include "net_progress_i.h"
#include "net_mem_block_i.h"

net_mem_block_t net_progress_buf_combine(net_progress_t progress);
uint32_t net_progress_buf_calc_size(net_progress_t progress);

net_progress_t
net_progress_auto_create(
    net_schedule_t schedule, const char * cmd, net_progress_runing_mode_t mode,
    net_progress_data_watch_fun_t data_watch_fun, void * ctx)
{
    net_driver_t driver;

    TAILQ_FOREACH(driver, &schedule->m_drivers, m_next_for_schedule) {
        if (driver->m_progress_init) {
            return net_progress_create(driver, cmd, mode, data_watch_fun, ctx);
        }
    }
    
    return NULL;
}

net_progress_t
net_progress_create(
    net_driver_t driver, const char * cmd, net_progress_runing_mode_t mode,
    net_progress_data_watch_fun_t data_watch_fun, void * ctx)
{
    net_schedule_t schedule = driver->m_schedule;

    net_progress_t progress = mem_alloc(schedule->m_alloc, sizeof(struct net_progress) + driver->m_progress_capacity);
    if (progress == NULL) {
        CPE_ERROR(schedule->m_em, "net: progress: create: alloc fail!");
        return NULL;
    }

    progress->m_driver = driver;
    progress->m_mode = mode;
    progress->m_mem_group = schedule->m_dft_mem_group;
    progress->m_data_watch_fun = data_watch_fun;
    progress->m_data_watch_ctx = ctx;
    progress->m_size = 0;
    TAILQ_INIT(&progress->m_blocks);
    
    if (driver->m_progress_init(progress, cmd, mode) != 0) {
        CPE_ERROR(
            schedule->m_em, "net: progress: create: start cmd fail, mode=%s, cmd=%s!",
            net_progress_runing_mode_str(mode),
            cmd);
        mem_free(schedule->m_alloc, progress);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&driver->m_progresses, progress, m_next_for_driver);
    return progress;
}

void net_progress_free(net_progress_t progress) {
    net_driver_t driver = progress->m_driver;
    net_schedule_t schedule = driver->m_schedule;

    driver->m_progress_fini(progress);

    TAILQ_REMOVE(&driver->m_progresses, progress, m_next_for_driver);

    mem_free(schedule->m_alloc, progress);
}

net_driver_t net_progress_driver(net_progress_t progress) {
    return progress->m_driver;
}

net_progress_runing_mode_t net_progress_runing_mode(net_progress_t progress) {
    return progress->m_mode;
}

void net_progress_buf_clear(net_progress_t progress) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    while(!TAILQ_EMPTY(&progress->m_blocks)) {
        net_mem_block_free(TAILQ_FIRST(&progress->m_blocks));
    }
    assert(progress->m_size == 0);
}

void net_progress_buf_consume(net_progress_t progress, uint32_t size) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    if (size == 0) return;
    
    assert(!TAILQ_EMPTY(&progress->m_blocks));
    assert(progress->m_size >= size);

    uint32_t left_size = size;
    while(left_size > 0) {
        net_mem_block_t block = TAILQ_FIRST(&progress->m_blocks);
        assert(block);

        if (block->m_len <= left_size) {
            left_size -= block->m_len;
            net_mem_block_free(block);
        }
        else {
            net_mem_block_erase(block, left_size);
            left_size = 0;
        }
    }
    
    assert(progress->m_size == net_progress_buf_calc_size(progress));
    
    /* if (progress->m_data_watcher_fun) { */
    /*     progress->m_data_watcher_fun(progress->m_data_watcher_ctx, progress, size); */
    /* } */
}

void * net_progress_buf_peak(net_progress_t progress, uint32_t * size) {
    net_mem_block_t block = TAILQ_FIRST(&progress->m_blocks);
    if (block == NULL) {
        *size = 0;
        return NULL;
    }
    else {
        *size = block->m_len;
        return block->m_data;
    }
}

int net_progress_buf_peak_with_size(net_progress_t progress, uint32_t require, void * * r_data) {
    assert(require > 0);

    if (progress->m_size < require) {
        *r_data = NULL;
        return 0;
    }

    net_mem_block_t block = TAILQ_FIRST(&progress->m_blocks);
    assert(block);

    void * data;
    if (block->m_len >= require) {
        *r_data = block->m_data;
        return 0;
    }

    block = net_progress_buf_combine(progress);
    if (block == NULL) {
        return -1;
    }

    *r_data = block->m_data;

    return 0;
}

int net_progress_buf_recv(net_progress_t progress, void * data, uint32_t * size) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    uint32_t capacity = *size;
    uint32_t received = 0;

    while(capacity > 0 && !TAILQ_EMPTY(&progress->m_blocks)) {
        net_mem_block_t block = TAILQ_FIRST(&progress->m_blocks);

        assert(block->m_len <= progress->m_size);

        uint32_t copy_len = cpe_min(block->m_len, capacity);
        memcpy(data, block->m_data, copy_len);

        capacity -= copy_len;
        data = ((char*)data) + copy_len;
        received += copy_len;

        if (copy_len == block->m_len) {
            net_mem_block_free(block);
        }
        else {
            net_mem_block_erase(block, copy_len);
        }
        assert(progress->m_size == net_progress_buf_calc_size(progress));
    }

    *size = received;

    return 0;
}

int net_progress_buf_by_sep(
    net_progress_t progress, const char * seps, void * * r_data, uint32_t *r_size)
{
    net_schedule_t schedule = progress->m_driver->m_schedule;

    uint32_t sz = 0;
    net_mem_block_t block = TAILQ_FIRST(&progress->m_blocks);
    while(block) {
        int block_pos;
        for(block_pos = 0; block_pos < block->m_len; ++block_pos) {
            int j;
            for(j = 0; seps[j] != 0; ++j) {
                if (block->m_data[block_pos] == seps[j]) {
                    if (block != TAILQ_FIRST(&progress->m_blocks)) { /*多块，需要合并 */
                        block_pos += sz;
                        
                        block = net_progress_buf_combine(progress);
                        if (block == NULL) {
                            return -1;
                        }
                    }

                    block->m_data[block_pos] = 0;
                    *r_data = block->m_data;
                    *r_size = block_pos + 1;
                    return 0;
                }
            }
        }
        sz += (uint32_t)block->m_len;
        block = TAILQ_NEXT(block, m_progress.m_next);
    }
    
    *r_data = NULL;
    *r_size = 0;
    return 0;
}

void * net_progress_data(net_progress_t progress) {
    return progress + 1;
}

net_progress_t net_progress_from_data(void * data) {
    return ((net_progress_t)data) - 1;
}

const char * net_progress_runing_mode_str(net_progress_runing_mode_t mode) {
    switch(mode) {
    case net_progress_runing_mode_read:
        return "read";
    case net_progress_runing_mode_write:
        return "write";
    }
}

net_mem_block_t net_progress_buf_combine(net_progress_t progress) {
    if (progress->m_size == 0) return NULL;
    
    net_mem_block_t block =
        net_mem_block_create(
            progress->m_mem_group, progress->m_id,
            progress->m_size, net_mem_alloc_capacity_at_least);
    if (block == NULL) {
        return NULL;
    }

    while(!TAILQ_EMPTY(&progress->m_blocks)) {
        net_mem_block_t old_block = TAILQ_FIRST(&progress->m_blocks);
        memcpy(block->m_data + block->m_len, old_block->m_data, old_block->m_len);
        block->m_len += old_block->m_len;
        net_mem_block_free(old_block);
    }

    net_mem_block_link_progress(block, progress);

    return block;
}

uint32_t net_progress_buf_calc_size(net_progress_t progress) {
    uint32_t size = 0;

    net_mem_block_t block;
    TAILQ_FOREACH(block, &progress->m_blocks, m_endpoint.m_next) {
        size += block->m_len;
    }
    
    return size;
}
