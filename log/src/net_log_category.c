#include "net_log_category_i.h"
#include "log_producer_manager.h"

net_log_category_t
net_log_category_create(net_log_schedule_t schedule, const char * name, uint8_t id) {
    if (id >= schedule->m_category_count) {
        uint8_t new_count = id < 16 ? 16 : id;
        net_log_category_t * new_categories = mem_calloc(schedule->m_alloc, sizeof(net_log_category_t) * new_count);
        if (new_categories == NULL) {
            CPE_ERROR(schedule->m_em, "log: create schedule %d: alloc categories buf fail!", id);
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
        CPE_ERROR(schedule->m_em, "log: create schedule %d: already created!", id);
        return NULL;
    }

    net_log_category_t category = mem_alloc(schedule->m_alloc, sizeof(struct net_log_category));
    if (category == NULL) {
        CPE_ERROR(schedule->m_em, "log: create schedule %d: alloc fail!", id);
        return NULL;
    }

    category->m_schedule = schedule;
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
        CPE_ERROR(schedule->m_em, "log: create schedule %d: create producer manager fail!", id);
        destroy_log_producer_config(config);
        mem_free(schedule->m_alloc, category);
        return NULL;
    }
    category->m_producer_manager->send_done_function = NULL;

    schedule->m_categories[id] = category;
    return 0;
}

void net_log_category_free(net_log_category_t category) {
    net_log_schedule_t schedule = category->m_schedule;

    assert(schedule->m_categories[category->m_id] == category);

    if (schedule->m_current_category == category) {
        schedule->m_current_category = NULL;
    }

    destroy_log_producer_manager(category->m_producer_manager);
    destroy_log_producer_config(category->m_producer_config);

    mem_free(schedule->m_alloc, category);
}

void net_log_category_network_recover(net_log_category_t category) {
    category->m_producer_manager->networkRecover = 1;
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


