#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/math_ex.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "cpe/pal/pal_string.h"
#include "net_driver_i.h"
#include "net_progress_i.h"
#include "net_mem_block_i.h"

static net_mem_block_t net_progress_buf_combine(net_progress_t progress);
static uint32_t net_progress_buf_calc_size(net_progress_t progress);
static void net_progress_buf_clear(net_progress_t progress);
static int net_progress_buf_on_supply(net_schedule_t schedule, net_progress_t progress);
static int net_progress_buf_on_consume(net_schedule_t schedule, net_progress_t progress);

net_progress_t
net_progress_auto_create(
    net_schedule_t schedule, const char * cmd, const char * argv[], net_progress_runing_mode_t mode,
    net_progress_update_fun_t update_fun, void * ctx, enum net_progress_debug_mode debug)
{
    net_driver_t driver;

    TAILQ_FOREACH(driver, &schedule->m_drivers, m_next_for_schedule) {
        if (driver->m_progress_init) {
            return net_progress_create(driver, cmd, argv, mode, update_fun, ctx, debug);
        }
    }
    
    return NULL;
}

net_progress_t
net_progress_create(
    net_driver_t driver, const char * cmd, const char * argv[], net_progress_runing_mode_t mode,
    net_progress_update_fun_t update_fun, void * ctx, enum net_progress_debug_mode debug)
{
    net_schedule_t schedule = driver->m_schedule;

    net_progress_t progress = mem_alloc(schedule->m_alloc, sizeof(struct net_progress) + driver->m_progress_capacity);
    if (progress == NULL) {
        CPE_ERROR(schedule->m_em, "net: progress: create: alloc fail!");
        return NULL;
    }

    progress->m_driver = driver;
    progress->m_id = ++schedule->m_endpoint_max_id;
    progress->m_mode = mode;
    progress->m_debug = debug;
    progress->m_cmd = NULL;
    progress->m_mem_group = schedule->m_dft_mem_group;
    progress->m_update_fun = update_fun;
    progress->m_update_ctx = ctx;
    progress->m_state = net_progress_state_runing;
    progress->m_block_size = 0;
    progress->m_tb = NULL;
    progress->m_exit_stat = 0;
    progress->m_errno = 0;
    progress->m_error_msg = NULL;

    TAILQ_INIT(&progress->m_blocks);

    progress->m_cmd = cpe_str_mem_dup(schedule->m_alloc, cmd);

    if (driver->m_progress_init(progress, cmd, argv, mode) != 0) {
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

    if (progress->m_tb) {
        net_mem_block_free(progress->m_tb);
        progress->m_tb = NULL;
    }

    net_progress_buf_clear(progress);

    if (progress->m_cmd) {
        mem_free(schedule->m_alloc, progress->m_cmd);
        progress->m_cmd = NULL;
    }
    
    if (progress->m_error_msg) {
        mem_free(schedule->m_alloc, progress->m_error_msg);
        progress->m_error_msg = NULL;
    }
    
    TAILQ_REMOVE(&driver->m_progresses, progress, m_next_for_driver);
    
    mem_free(schedule->m_alloc, progress);
}

uint32_t net_progress_id(net_progress_t progress) {
    return progress->m_id;
}

net_driver_t net_progress_driver(net_progress_t progress) {
    return progress->m_driver;
}

net_progress_runing_mode_t net_progress_runing_mode(net_progress_t progress) {
    return progress->m_mode;
}

net_progress_state_t net_progress_state(net_progress_t progress) {
    return progress->m_state;
}

int net_progress_exit_state(net_progress_t progress) {
    return progress->m_exit_stat;
}

int net_progress_errno(net_progress_t progress) {
    return progress->m_errno;
}

const char * net_progress_error_msg(net_progress_t progress) {
    return progress->m_error_msg;
}

static net_mem_block_t
net_progress_block_alloc(net_progress_t progress, uint32_t * capacity, net_mem_alloc_capacity_policy_t policy) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    assert(progress->m_tb == NULL);

    progress->m_tb = net_mem_block_create(progress->m_mem_group, progress->m_id, *capacity, policy);
    if (progress->m_tb == NULL) {
        return NULL;
    }

    *capacity = progress->m_tb->m_capacity;
    return progress->m_tb;
}

void * net_progress_buf_alloc(net_progress_t progress, uint32_t size) {
    return net_progress_buf_alloc_at_least(progress, &size);
}

void * net_progress_buf_alloc_at_least(net_progress_t progress, uint32_t * inout_size) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    assert(inout_size);

    net_mem_block_t tb = net_progress_block_alloc(progress, inout_size, net_mem_alloc_capacity_at_least);
    if (tb == NULL) {
        return NULL;
    }
    
    *inout_size = tb->m_capacity;

    return tb->m_data;
}

void * net_progress_buf_alloc_suggest(net_progress_t progress, uint32_t * inout_size) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    assert(inout_size);

    net_mem_block_t tb = net_progress_block_alloc(progress, inout_size, net_mem_alloc_capacity_suggest);
    if (tb == NULL) {
        return NULL;
    }
    
    *inout_size = tb->m_capacity;

    return tb->m_data;
}

void net_progress_buf_release(net_progress_t progress) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    assert(progress->m_tb);
    if (progress->m_tb) {
        net_mem_block_free(progress->m_tb);
        progress->m_tb = NULL;
    }
}

uint8_t net_progress_buf_validate(net_progress_t progress, void const * buf, uint32_t capacity) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    if (progress->m_tb == NULL) {
        CPE_ERROR(
            schedule->m_em, "core: %s: buf validate, state=%s",
            net_progress_dump(&schedule->m_tmp_buffer, progress),
            net_progress_state_str(progress->m_state));
        return -1;
    }

    if (buf < (void *)progress->m_tb->m_data
        || ((const uint8_t *)buf) + capacity > progress->m_tb->m_data + progress->m_tb->m_capacity) return 0;

    return 1;
}

int net_progress_buf_supply(net_progress_t progress, uint32_t size) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    if (progress->m_tb == NULL) {
        CPE_ERROR(
            schedule->m_em, "core: %s: supply no buffer, state=%s",
            net_progress_dump(&schedule->m_tmp_buffer, progress),
            net_progress_state_str(progress->m_state));
        return -1;
    }
    
    if (size > 0) {
        assert(size <= progress->m_tb->m_capacity);
        progress->m_tb->m_len = size;
        net_mem_block_link_progress(progress->m_tb, progress);
        progress->m_tb = NULL;
        return net_progress_buf_on_supply(schedule, progress);
    }
    else {
        net_mem_block_free(progress->m_tb);
        progress->m_tb = NULL;
        return 0;
    }
}

int net_progress_buf_append(net_progress_t progress, void const * i_data, uint32_t size) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    if (size == 0) return 0;

    net_mem_block_t last_block = TAILQ_LAST(&progress->m_blocks, net_mem_block_list);
    if (last_block && (last_block->m_capacity - last_block->m_len) >= size) {
        net_mem_block_append(last_block, i_data, size);
    }
    else {
        net_mem_block_t new_block = net_mem_block_create(
            progress->m_mem_group, progress->m_id, size, net_mem_alloc_capacity_at_least);
        if (new_block == NULL) return -1;

        memcpy(new_block->m_data, i_data, size);
        new_block->m_len = size;

        net_mem_block_link_progress(new_block, progress);
    }

    return net_progress_buf_on_supply(schedule, progress);
}

int net_progress_buf_append_char(net_progress_t progress, uint8_t c) {
    return net_progress_buf_append(progress, &c, sizeof(c));
}

int net_progress_set_complete(net_progress_t progress, int exit_stat) {
    net_schedule_t schedule = progress->m_driver->m_schedule;
    
    switch(progress->m_state) {
    case net_progress_state_runing:
        break;
    case net_progress_state_complete:
    case net_progress_state_error:
        CPE_ERROR(
            schedule->m_em, "core: %s: complete: can`t complete in state=%s",
            net_progress_dump(&schedule->m_tmp_buffer, progress),
            net_progress_state_str(progress->m_state));
        return -1;
    }

    if (progress->m_debug != net_progress_debug_none) {
        CPE_INFO(
            schedule->m_em, "core: %s: complete, exit-stat=%d",
            net_progress_dump(&schedule->m_tmp_buffer, progress), exit_stat);
    }

    progress->m_exit_stat = exit_stat;
    progress->m_state = net_progress_state_complete;

    if (progress->m_update_fun) {
        progress->m_update_fun(progress->m_update_ctx, progress);
    }
    
    return 0;
}

int net_progress_set_error(net_progress_t progress, int err, const char * error_msg) {
    net_schedule_t schedule = progress->m_driver->m_schedule;
    
    switch(progress->m_state) {
    case net_progress_state_runing:
        break;
    case net_progress_state_complete:
    case net_progress_state_error:
        CPE_ERROR(
            schedule->m_em, "core: %s: error: can`t error in state=%s",
            net_progress_dump(&schedule->m_tmp_buffer, progress),
            net_progress_state_str(progress->m_state));
        return -1;
    }

    if (progress->m_debug != net_progress_debug_none) {
        CPE_INFO(
            schedule->m_em, "core: %s: error: errno=%d (%s), error-msg=%s",
            net_progress_dump(&schedule->m_tmp_buffer, progress),
            err, strerror(err), error_msg);
    }

    progress->m_errno = err;
    assert(progress->m_error_msg == NULL);
    progress->m_error_msg = error_msg ? cpe_str_mem_dup(schedule->m_alloc, error_msg) : NULL;
    progress->m_state = net_progress_state_error;

    if (progress->m_update_fun) {
        progress->m_update_fun(progress->m_update_ctx, progress);
    }
    
    return 0;
}

uint32_t net_progress_buf_size(net_progress_t progress) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    assert(net_progress_buf_calc_size(progress) == progress->m_block_size);

    return progress->m_block_size;
}

void net_progress_buf_clear(net_progress_t progress) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    while(!TAILQ_EMPTY(&progress->m_blocks)) {
        net_mem_block_free(TAILQ_FIRST(&progress->m_blocks));
    }
    assert(progress->m_block_size == 0);
}

void net_progress_buf_consume(net_progress_t progress, uint32_t size) {
    net_schedule_t schedule = progress->m_driver->m_schedule;

    if (size == 0) return;
    
    assert(!TAILQ_EMPTY(&progress->m_blocks));
    assert(progress->m_block_size >= size);

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
    
    assert(progress->m_block_size == net_progress_buf_calc_size(progress));

    net_progress_buf_on_consume(schedule, progress);
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

    if (progress->m_block_size < require) {
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

        assert(block->m_len <= progress->m_block_size);

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
        assert(progress->m_block_size == net_progress_buf_calc_size(progress));
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

void net_progress_print(write_stream_t ws, net_progress_t progress) {
    stream_printf(ws, "[%d: %s]", progress->m_id, progress->m_cmd);
}

const char * net_progress_dump(mem_buffer_t buff, net_progress_t progress) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buff);

    mem_buffer_clear_data(buff);
    net_progress_print((write_stream_t)&stream, progress);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buff, 0);
}

const char * net_progress_runing_mode_str(net_progress_runing_mode_t mode) {
    switch(mode) {
    case net_progress_runing_mode_read:
        return "read";
    case net_progress_runing_mode_write:
        return "write";
    }
}

const char * net_progress_state_str(net_progress_state_t state) {
    switch(state) {
    case net_progress_state_runing:
        return "runing";
    case net_progress_state_complete:
        return "complete";
    case net_progress_state_error:
        return "error";
    }
}

net_mem_block_t net_progress_buf_combine(net_progress_t progress) {
    if (progress->m_block_size == 0) return NULL;
    
    net_mem_block_t block =
        net_mem_block_create(
            progress->m_mem_group, progress->m_id,
            progress->m_block_size, net_mem_alloc_capacity_at_least);
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

static int net_progress_buf_on_supply(net_schedule_t schedule, net_progress_t progress) {
    switch(progress->m_mode) {
    case net_progress_runing_mode_read:
        if (progress->m_update_fun) {
            progress->m_update_fun(progress->m_update_ctx, progress);
        }
        break;
    case net_progress_runing_mode_write:
        break;
    }

    return 0;
}

static int net_progress_buf_on_consume(net_schedule_t schedule, net_progress_t progress) {
    return 0;
}
