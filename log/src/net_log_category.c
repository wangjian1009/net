#include <assert.h>
#include "md5.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_log_category_i.h"
#include "net_log_flusher_i.h"
#include "net_log_sender_i.h"
#include "net_log_request.h"
#include "net_log_request_pipe.h"
#include "net_log_builder.h"

static char * net_log_category_get_pack_id(net_log_schedule_t schedule, const char * configName, const char * ip);

net_log_category_t
net_log_category_create(net_log_schedule_t schedule, net_log_flusher_t flusher, net_log_sender_t sender, const char * name, uint8_t id) {
    if (schedule->m_state != net_log_schedule_state_init) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: schedule in state %s, can`t create !", id, name,
            net_log_schedule_state_str(schedule->m_state));
        return NULL;
    }

    if (sender == NULL) {
        if (flusher != NULL) { /*flushe需要通过主线程发送数据，需要创建主线程通道 */
            if (net_log_schedule_init_main_thread_pipe(schedule) != 0) {
                CPE_ERROR(schedule->m_em, "log: category [%d]%s: init main thread pipe fail!", id, name);
                return NULL;
            }
        }
        else { /*没有flusher，数据从主线程直接发送，只需要创建manager */
            if (net_log_schedule_init_main_thread_mgr(schedule) != 0) {
                CPE_ERROR(schedule->m_em, "log: category [%d]%s: init main thread mgr fail!", id, name);
                return NULL;
            }
        }
    }
    
    if (id >= schedule->m_category_count) {
        uint8_t new_count = id < 16 ? 16 : id;
        net_log_category_t * new_categories = mem_calloc(schedule->m_alloc, sizeof(net_log_category_t) * new_count);
        if (new_categories == NULL) {
            CPE_ERROR(schedule->m_em, "log: category [%d]%s: alloc categories buf fail!", id, name);
            return NULL;
        }

        if (schedule->m_categories) {
            memcpy(new_categories, schedule->m_categories, sizeof(net_log_category_t) * schedule->m_category_count);
            mem_free(schedule->m_alloc, schedule->m_categories);
        }

        schedule->m_categories = new_categories;
        schedule->m_category_count = new_count;
    }

    if (schedule->m_categories[id] == NULL) {
        CPE_ERROR(schedule->m_em, "log: category [%d]%s: already created!", id, name);
        return NULL;
    }

    net_log_category_t category = mem_alloc(schedule->m_alloc, sizeof(struct net_log_category));
    if (category == NULL) {
        CPE_ERROR(schedule->m_em, "log: category [%d]%s: alloc fail!", id, name);
        return NULL;
    }

    category->m_schedule = schedule;
    category->m_flusher = flusher;
    category->m_sender = sender;
    cpe_str_dup(category->m_name, sizeof(category->m_name), name);
    category->m_id = id;
    category->m_networkRecover = 0;
    category->m_totalBufferSize = 0;
    category->m_builder = NULL;
    category->m_firstLogTime = 0;
    category->m_pack_prefix = NULL;
    category->m_pack_index = 0;

    category->m_cfg_topic = NULL;
    category->m_cfg_tags = NULL;
    category->m_cfg_tag_count = 0;
    category->m_cfg_tag_capacity = 0;
    category->m_cfg_compress = net_log_compress_lz4;
    category->m_cfg_bytes_per_package = 3 * 1024 * 1024;
    category->m_cfg_count_per_package = 2048;
    category->m_cfg_timeout_ms = 3000;
    category->m_cfg_max_buffer_bytes = 64 * 1024 * 1024;
    category->m_cfg_connect_timeout_s = 10;
    category->m_cfg_send_timeout_s = 15;
    /* category->destroySenderWaitTimeoutSec = 1; */
    /* category->destroyFlusherWaitTimeoutSec = 1; */
    
    category->m_pack_prefix = net_log_category_get_pack_id(schedule, category->m_name, schedule->m_cfg_source);
    if (category->m_pack_prefix == NULL) {
        CPE_ERROR(schedule->m_em, "log: category [%d]%s: create pack prefix fail!", id, name);
        mem_free(schedule->m_alloc, category);
        return NULL;
    }

    schedule->m_categories[id] = category;

    if (flusher) {
        TAILQ_INSERT_TAIL(&flusher->m_categories, category, m_next_for_flusher);
    }

    if (sender) {
        TAILQ_INSERT_TAIL(&sender->m_categories, category, m_next_for_sender);
    }
    
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: category [%d]%s: create success, flusher %s, sender %s!",
            id, name,
            flusher ? flusher->m_name : "N/A",
            sender ? sender->m_name : "N/A");
    }
    
    return 0;
}

void net_log_category_free(net_log_category_t category) {
    net_log_schedule_t schedule = category->m_schedule;

    assert(schedule->m_state == net_log_schedule_state_init);
    assert(schedule->m_categories[category->m_id] == category);

    if (schedule->m_current_category == category) {
        schedule->m_current_category = NULL;
    }

    schedule->m_categories[category->m_id] = NULL;

    if (category->m_flusher) {
        TAILQ_REMOVE(&category->m_flusher->m_categories, category, m_next_for_flusher);
    }

    if (category->m_sender) {
        TAILQ_REMOVE(&category->m_sender->m_categories, category, m_next_for_sender);
    }

    mem_free(schedule->m_alloc, category->m_pack_prefix);

    /*config*/
    if (category->m_cfg_topic) {
        mem_free(schedule->m_alloc, category->m_cfg_topic);
        category->m_cfg_topic = NULL;
    }
    
    if (category->m_cfg_tags) {
        uint16_t i = 0;
        for (; i < category->m_cfg_tag_count; ++i) {
            mem_free(schedule->m_alloc, category->m_cfg_tags[i].m_key);
            mem_free(schedule->m_alloc, category->m_cfg_tags[i].m_value);
        }
        mem_free(schedule->m_alloc, category->m_cfg_tags);
        category->m_cfg_tags = NULL;
        category->m_cfg_tag_count = 0;
    }
    
    mem_free(schedule->m_alloc, category);
}

void net_log_category_network_recover(net_log_category_t category) {
    category->m_networkRecover = 1;
}

net_log_request_param_t
net_log_category_build_request(net_log_category_t category, net_log_group_builder_t builder) {
    net_log_schedule_t schedule = category->m_schedule;

    category->m_totalBufferSize -= builder->loggroup_size;

    uint16_t i;
    for (i = 0; i < category->m_cfg_tag_count; ++i) {
        net_log_category_cfg_tag_t tag = category->m_cfg_tags + i;
        add_tag(builder, tag->m_key, strlen(tag->m_key), tag->m_value, strlen(tag->m_value));
    }

    if (category->m_cfg_topic != NULL) {
        add_topic(builder, category->m_cfg_topic, strlen(category->m_cfg_topic));
    }
    
    if (schedule->m_cfg_source != NULL) {
        add_source(builder, schedule->m_cfg_source, strlen(schedule->m_cfg_source));
    }

    if (category->m_pack_prefix != NULL) {
        add_pack_id(builder, category->m_pack_prefix, strlen(category->m_pack_prefix), category->m_pack_index++);
    }

    lz4_log_buf * lz4_buf = NULL;
    if (category->m_cfg_compress == net_log_compress_lz4) {
        lz4_buf = serialize_to_proto_buf_with_malloc_lz4(builder);
    }
    else {
        lz4_buf = serialize_to_proto_buf_with_malloc_no_lz4(builder);
    }

    if (lz4_buf == NULL) {
        CPE_ERROR(schedule->m_em, "serialize loggroup to proto buf with lz4 failed");
        return NULL;
    }

    category->m_totalBufferSize += lz4_buf->length;

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: category [%d]%s: push loggroup to sender, loggroup size %d, lz4 size %d, now buffer size %d",
            category->m_id, category->m_name,
            (int)lz4_buf->raw_length, (int)lz4_buf->length, (int)category->m_totalBufferSize);
    }

    net_log_request_param_t send_param = net_log_request_param_create(category, lz4_buf, builder->builder_time);
    if (send_param == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: build send params: create fail!",
            category->m_id, category->m_name);
        return NULL;
    }

    return send_param;
}

int net_log_category_commit_request(net_log_category_t category, net_log_request_param_t send_param, uint8_t in_main_thread) {
    net_log_schedule_t schedule = category->m_schedule;

    if (category->m_sender) {
        if (net_log_request_pipe_queue(category->m_sender->m_request_pipe, send_param) != 0) {
            CPE_ERROR(
                schedule->m_em, "log: category [%d]%s: commit request to %s fail!",
                category->m_id, category->m_name, category->m_sender->m_name);
            return -1;
        }
        return 0;
    }
    else if (in_main_thread) { /*在主线程中，直接发送 */
        assert(schedule->m_main_thread_request_mgr);

        net_log_request_t request = net_log_request_create(schedule->m_main_thread_request_mgr, send_param);
        if (request == NULL) {
            CPE_ERROR(
                schedule->m_em, "log: category [%d]%s: process request at main-thread fail!",
                category->m_id, category->m_name);
            return -1;
        }

        return 0;
    }
    else { /*提交到主线程处理 */
        assert(schedule->m_main_thread_request_pipe);
        if (net_log_request_pipe_queue(schedule->m_main_thread_request_pipe, send_param) != 0) {
            CPE_ERROR(
                schedule->m_em, "log: category [%d]%s: commit request to main-thread fail!",
                category->m_id, category->m_name);
            return -1;
        }
        return 0;
    }
}


int net_log_category_set_topic(net_log_category_t category, const char * topic) {
    net_log_schedule_t schedule = category->m_schedule;
    assert(schedule->m_state == net_log_schedule_state_init);

    char * new_topic = cpe_str_mem_dup(schedule->m_alloc, topic);
    if (new_topic == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: set topic: dup topic '%s' fail!",
            category->m_id, category->m_name, topic);
        return -1; 
    }

    if (category->m_cfg_topic) {
        mem_free(schedule->m_alloc, category->m_cfg_topic);
    }

    category->m_cfg_topic = new_topic;

    return 0;
}

int net_log_category_add_global_tag(net_log_category_t category, const char * key, const char * value) {
    net_log_schedule_t schedule = category->m_schedule;
    assert(schedule->m_state == net_log_schedule_state_init);
    
    if (category->m_cfg_tag_count >= category->m_cfg_tag_capacity) {
        uint16_t new_capacity = category->m_cfg_tag_capacity < 4 ? 4 : (category->m_cfg_tag_capacity * 2);
        net_log_category_cfg_tag_t new_tags = mem_alloc(schedule->m_alloc, sizeof(struct net_log_category_cfg_tag) * new_capacity);
        if (new_tags == NULL) {
            CPE_ERROR(
                schedule->m_em, "log: category [%d]%s: add cfg tag: resize capacity fail, new-capacity=%d!",
                category->m_id, category->m_name, new_capacity);
            return -1; 
        }

        if (category->m_cfg_tags) {
            memcpy(new_tags, category->m_cfg_tags, sizeof(struct net_log_category_cfg_tag) * category->m_cfg_tag_count);
            mem_free(schedule->m_alloc, category->m_cfg_tags);
        }

        category->m_cfg_tags = new_tags;
        category->m_cfg_tag_capacity = new_capacity;
    }

    char * new_key = cpe_str_mem_dup(schedule->m_alloc, key);
    if (new_key == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: add cfg tag: dup key '%s' fail!",
            category->m_id, category->m_name, key);
        return -1; 
    }

    char * new_value = cpe_str_mem_dup(schedule->m_alloc, value);
    if (new_value == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: add cfg tag: dup value '%s' fail!",
            category->m_id, category->m_name, value);
        mem_free(schedule->m_alloc, new_key);
        return -1; 
    }

    net_log_category_cfg_tag_t tag = category->m_cfg_tags + category->m_cfg_tag_count;
    tag->m_key = new_key;
    tag->m_value = new_value;
    category->m_cfg_tag_count ++;

    return 0;
}

int net_log_category_add_log(
    net_log_category_t category, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens)
{
    net_log_schedule_t schedule = category->m_schedule;

    if (category->m_totalBufferSize > category->m_cfg_max_buffer_bytes) {
        return -1;
    }
    
    if (category->m_builder == NULL) {
        int32_t now_time = time(NULL);

        category->m_builder = log_group_create(schedule);
        category->m_firstLogTime = now_time;
        category->m_builder->private_value = category;
    }

    add_log_full(category->m_builder, (uint32_t)time(NULL), pair_count, keys, key_lens, values, val_lens);

    net_log_group_builder_t builder = category->m_builder;

    int32_t nowTime = time(NULL);
    if (category->m_builder->loggroup_size < category->m_cfg_bytes_per_package
        && nowTime - category->m_firstLogTime < category->m_cfg_timeout_ms / 1000
        && category->m_builder->grp->n_logs < category->m_cfg_count_per_package)
    {
        return 0;
    }

    category->m_builder = NULL;

    size_t loggroup_size = builder->loggroup_size;

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "try push loggroup to flusher, size : %d, log count %d",
            (int)builder->loggroup_size, (int)builder->grp->n_logs);
    }

    if (category->m_flusher) { /*异步flush */
        if (net_log_flusher_queue(category->m_flusher, builder) != 0) {
            CPE_ERROR(schedule->m_em, "try push loggroup to flusher failed, force drop this log group");
            log_group_destroy(builder);
        }
        else {
            category->m_totalBufferSize += loggroup_size;
        }
    }
    else { /*同步flush */
        category->m_totalBufferSize += loggroup_size;

        net_log_request_param_t send_param = net_log_category_build_request(category, builder);
        log_group_destroy(builder);
        
        if (send_param) {
            if (net_log_category_commit_request(category, send_param, 0) != 0) {
                net_log_request_param_free(send_param);
            }
        }
    }
    
    return 0;
}

char * net_log_category_get_pack_id(net_log_schedule_t schedule, const char * configName, const char * ip) {
    unsigned char md5Buf[16];
    mbedtls_md5((const unsigned char *)configName, strlen(configName), md5Buf);
    int loop = 0;
    char * val = (char *)mem_alloc(schedule->m_alloc, sizeof(char) * 32);
    memset(val, 0, sizeof(char) * 32);
    for(; loop < 8; ++loop) {
        unsigned char a = ((md5Buf[loop])>>4) & 0xF, b = (md5Buf[loop]) & 0xF;
        val[loop<<1] = a > 9 ? (a - 10 + 'A') : (a + '0');
        val[(loop<<1)|1] = b > 9 ? (b - 10 + 'A') : (b + '0');
    }
    return val;
}
