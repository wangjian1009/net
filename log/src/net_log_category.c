#include "net_log_category_i.h"

net_log_category_t
net_log_category_create(net_log_schedule_t schedule, const char * name, uint8_t id) {
    if (id >= schedule->m_category_count) {
        
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
    category->m_log_producer = NULL;
    
    log_producer_config * config = create_log_producer_config();
    //log_producer_config_set_endpoint(config, s_aliyun_log_ep);

    //TODO:
    /* char project[32]; */
    /* snprintf(project, sizeof(project), "%s-%s", s_aliyun_log_project, log_env); */
    //log_producer_config_set_project(config, project);
    //log_producer_config_set_logstore(config, logstore);

    //log_producer_config_set_access_id(config, s_aliyun_log_key_id);
    //log_producer_config_set_access_key(config, s_aliyun_log_key_secrt);

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

    category->m_log_producer = create_log_producer(config, NULL/*net_log_on_send_done*/);
    if (category->m_log_producer == NULL) {
        CPE_ERROR(schedule->m_em, "log: create schedule %d: create producer fail!", id);
        mem_free(schedule->m_alloc, category);
        return NULL;
    }

    schedule->m_categories[id] = category;
    
    return 0;
}

void net_log_category_free(net_log_category_t category) {
    net_log_schedule_t schedule = category->m_schedule;

    assert(schedule->m_categories[category->m_id] == category);

    if (schedule->m_current_category == category) {
        schedule->m_current_category = NULL;
    }

    destroy_log_producer(category->m_log_producer);

    mem_free(schedule->m_alloc, category);
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


