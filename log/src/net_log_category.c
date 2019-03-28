#include "cpe/utils/string_utils.h"
#include "net_log_category_i.h"
#include "net_log_flusher_i.h"
#include "net_log_sender_i.h"
#include "net_log_request.h"
#include "net_log_request_pipe.h"
#include "log_producer_manager.h"

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
    category->m_producer_config = NULL;
    category->m_producer_manager = NULL;
    
    log_producer_config * config = create_log_producer_config(category);
    log_producer_config_set_logstore(config, name);

    //log_producer_config_set_topic(config, "test_topic");

    // set resource params
    log_producer_config_set_packet_log_bytes(config, 1*1024*1024);
    log_producer_config_set_packet_log_count(config, 512);
    log_producer_config_set_packet_timeout(config, 3000);
    log_producer_config_set_max_buffer_limit(config, 4*1024*1024);

    // set send thread count
    log_producer_config_set_send_thread_count(config, 1);

    // set compress type : lz4
    log_producer_config_set_compress_type(config, 1);

    // set timeout
    log_producer_config_set_connect_timeout_sec(config, 10);
    log_producer_config_set_send_timeout_sec(config, 15);
    log_producer_config_set_destroy_flusher_wait_sec(config, 1);
    log_producer_config_set_destroy_sender_wait_sec(config, 1);

    // set interface
    log_producer_config_set_net_interface(config, NULL);

    // create manager
    category->m_producer_config = config;
    category->m_producer_manager = create_log_producer_manager(category, config);
    if (category->m_producer_manager == NULL) {
        CPE_ERROR(schedule->m_em, "log: category [%d]%s: create producer manager fail!", id, name);
        destroy_log_producer_config(config);
        mem_free(schedule->m_alloc, category);
        return NULL;
    }
    category->m_producer_manager->send_done_function = NULL;

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
    
    destroy_log_producer_manager(category->m_producer_manager);
    destroy_log_producer_config(category->m_producer_config);

    mem_free(schedule->m_alloc, category);
}

void net_log_category_network_recover(net_log_category_t category) {
    category->m_producer_manager->networkRecover = 1;
}

int net_log_category_start(net_log_category_t category) {
    return 0;
}

void net_log_category_notify_stop(net_log_category_t category) {
}

void net_log_category_wait_stop(net_log_category_t category) {
}

log_producer_send_param_t
net_log_category_build_request(net_log_category_t category, log_group_builder_t builder) {
    net_log_schedule_t schedule = category->m_schedule;
    log_producer_manager * producer_manager = category->m_producer_manager;

    CS_ENTER(producer_manager->lock);
    producer_manager->totalBufferSize -= builder->loggroup_size;
    CS_LEAVE(producer_manager->lock);

    log_producer_config * config = producer_manager->producer_config;
    int i = 0;
    for (i = 0; i < config->tagCount; ++i) {
        add_tag(builder, config->tags[i].key, strlen(config->tags[i].key), config->tags[i].value, strlen(config->tags[i].value));
    }

    if (config->topic != NULL) {
        add_topic(builder, config->topic, strlen(config->topic));
    }
    
    if (producer_manager->source != NULL) {
        add_source(builder, producer_manager->source, strlen(producer_manager->source));
    }

    if (producer_manager->pack_prefix != NULL) {
        add_pack_id(builder, producer_manager->pack_prefix, strlen(producer_manager->pack_prefix), producer_manager->pack_index++);
    }

    lz4_log_buf * lz4_buf = NULL;
    if (config->compressType == 1) {
        lz4_buf = serialize_to_proto_buf_with_malloc_lz4(builder);
    }
    else {
        lz4_buf = serialize_to_proto_buf_with_malloc_no_lz4(builder);
    }

    if (lz4_buf == NULL) {
        CPE_ERROR(schedule->m_em, "serialize loggroup to proto buf with lz4 failed");
        return NULL;
    }

    CS_ENTER(producer_manager->lock);
    producer_manager->totalBufferSize += lz4_buf->length;
    CS_LEAVE(producer_manager->lock);

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: category [%d]%s: push loggroup to sender, loggroup size %d, lz4 size %d, now buffer size %d",
            category->m_id, category->m_name,
            (int)lz4_buf->raw_length, (int)lz4_buf->length, (int)producer_manager->totalBufferSize);
    }

    log_producer_send_param_t send_param =
        create_log_producer_send_param(config, producer_manager, lz4_buf, builder->builder_time);
    if (send_param == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: build send params: create fail!",
            category->m_id, category->m_name);
        return NULL;
    }

    return send_param;
}

int net_log_category_commit_request(net_log_category_t category, log_producer_send_param_t send_param, uint8_t in_main_thread) {
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

/* static void net_log_on_send_done( */
/*     const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, */
/*     const char * req_id, const char * message, const unsigned char * raw_buffer) */
/* { */
/*     if (s_logger == NULL) return; */

/*     sfox_agent_t agent = s_logger->m_chain->m_agent; */

/*     if (result == LOG_PRODUCER_OK) { */
/*         if (agent->m_debug) { */
/*             CPE_INFO( */
/*                 agent->m_em, */
/*                 "sfox: logger: %s: send success, result : %d, log bytes : %d, compressed bytes : %d, request id : %s", */
/*                 config_name, (result), */
/*                 (int)log_bytes, (int)compressed_bytes, req_id); */
/*         } */
/*     } */
/*     else { */
/*         CPE_ERROR( */
/*             agent->m_em, */
/*             "sfox: logger: %s: send fail, result : %d, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s", */
/*             config_name, (result), */
/*             (int)log_bytes, (int)compressed_bytes, req_id, message); */
/*     } */
/* } */
