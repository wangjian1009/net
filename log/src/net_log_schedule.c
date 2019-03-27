#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/md5.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_log_schedule_i.h"
#include "net_log_category_i.h"

net_log_schedule_t net_log_schedule_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t net_schedule, net_driver_t net_driver)
{
    net_log_schedule_t schedule = mem_alloc(alloc, sizeof(struct net_log_schedule));
    if (schedule == NULL) {
        CPE_ERROR(em, "log: schedule alloc fail");
        return NULL;
    }

    if (log_producer_env_init(LOG_GLOBAL_ALL) != LOG_PRODUCER_OK) {
        CPE_ERROR(em, "log: env init fail!");
        mem_free(alloc, schedule);
        return NULL;
    }
    
    bzero(schedule, sizeof(*schedule));

    schedule->m_alloc = alloc;
    schedule->m_em = em;
    schedule->m_debug = 0;
    schedule->m_net_schedule = net_schedule;
    schedule->m_net_driver = net_driver;

    mem_buffer_init(&schedule->m_kv_buffer, alloc);

    return schedule;
}

void net_log_schedule_free(net_log_schedule_t schedule) {
    uint8_t i;
    for(i = 0; i < schedule->m_category_count; ++i) {
        if (schedule->m_categories[i]) {
        }
    }
    
    /* uint32_t i; */
    /* for(i = 0; i < CPE_ARRAY_SIZE(logger->m_log_producer); ++i) { */
    /*     if (logger->m_log_producer[i]) { */
    /*         destroy_log_producer(logger->m_log_producer[i]); */
    /*     } */
    /* } */

    
    log_producer_env_destroy();

    mem_buffer_clear(&schedule->m_kv_buffer);

    mem_free(schedule->m_alloc, schedule);
}

uint8_t net_log_schedule_debug(net_log_schedule_t schedule) {
    return schedule->m_debug;
}

void net_log_schedule_set_debug(net_log_schedule_t schedule, uint8_t debug) {
    schedule->m_debug = debug;
}

/* static int sfox_chain_logger_add_category( */
/*     sfox_chain_logger_t logger, const char * log_env, const char * logstore, sfox_chain_log_type_t log_type) */
/* { */
/*     sfox_chain_t chain = logger->m_chain; */
/*     sfox_agent_t agent = chain->m_agent; */

/*     assert((int)log_type < CPE_ARRAY_SIZE(logger->m_log_producer)); */
/*     assert(logger->m_log_producer[log_type] == NULL); */

/*     log_producer_config * config = create_log_producer_config(); */
/*     log_producer_config_set_endpoint(config, s_aliyun_log_ep); */

/*     char project[32]; */
/*     snprintf(project, sizeof(project), "%s-%s", s_aliyun_log_project, log_env); */
/*     log_producer_config_set_project(config, project); */
/*     log_producer_config_set_logstore(config, logstore); */

/*     log_producer_config_set_access_id(config, s_aliyun_log_key_id); */
/*     log_producer_config_set_access_key(config, s_aliyun_log_key_secrt); */

/*     //log_producer_config_set_topic(config, "test_topic"); */

/*     // set resource params */
/*     log_producer_config_set_packet_log_bytes(config, 1*1024*1024); */
/*     log_producer_config_set_packet_log_count(config, 512); */
/*     log_producer_config_set_packet_timeout(config, 3000); */
/*     log_producer_config_set_max_buffer_limit(config, 4*1024*1024); */

/*     // set send thread count */
/*     log_producer_config_set_send_thread_count(config, 1); */

/*     // set compress type : lz4 */
/*     log_producer_config_set_compress_type(config, 1); */

/*     // set timeout */
/*     log_producer_config_set_connect_timeout_sec(config, 10); */
/*     log_producer_config_set_send_timeout_sec(config, 15); */
/*     log_producer_config_set_destroy_flusher_wait_sec(config, 1); */
/*     log_producer_config_set_destroy_sender_wait_sec(config, 1); */

/*     // set interface */
/*     log_producer_config_set_net_interface(config, NULL); */

/*     logger->m_log_producer[log_type] = create_log_producer(config, sfox_chain_log_on_send_done); */
/*     if (logger->m_log_producer[log_type] == NULL) { */
/*         CPE_ERROR(agent->m_em, "chain: logger: create producer fail!"); */
/*         return -1; */
/*     } */

/*     return 0; */
/* } */


void net_log_begin(net_log_schedule_t schedule, uint8_t log_type) {
    schedule->m_kv_count = 0;
    mem_buffer_clear_data(&schedule->m_kv_buffer);
    schedule->m_current_category = schedule->m_categories[log_type];
}

void net_log_append_int32(net_log_schedule_t schedule, const char * name, int32_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    net_log_append_str(schedule, name, buf);
}

void net_log_append_uint32(net_log_schedule_t schedule, const char * name, uint32_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    net_log_append_str(schedule, name, buf);
}

void net_log_append_int64(net_log_schedule_t schedule, const char * name, int64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), FMT_INT64_T, value);
    net_log_append_str(schedule, name, buf);
}

void net_log_append_uint64(net_log_schedule_t schedule, const char * name, uint64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), FMT_UINT64_T, value);
    net_log_append_str(schedule, name, buf);
}

void net_log_append_str(net_log_schedule_t schedule, const char * name, const char * value) {
    if (schedule->m_kv_count >= schedule->m_kv_capacity) {
        CPE_ERROR(schedule->m_em, "chain: log: append value overflow!");
        return;
    }

    int32_t pos = schedule->m_kv_count++;

    schedule->m_keys[pos] = mem_buffer_strdup(&schedule->m_kv_buffer, name);
    schedule->m_keys_len[pos] = strlen(name);

    schedule->m_values[pos] = mem_buffer_strdup(&schedule->m_kv_buffer, value);
    schedule->m_values_len[pos] = strlen(value);
}

void net_log_append_md5(net_log_schedule_t schedule, const char * name, cpe_md5_value_t value) {
    net_log_append_str(schedule, name, cpe_md5_dump(value, net_log_schedule_tmp_buffer(schedule)));
}

void net_log_append_net_address(net_log_schedule_t schedule, const char * name, net_address_t address) {
    net_log_append_str(schedule, name, net_address_dump(net_log_schedule_tmp_buffer(schedule), address));
}

void net_log_commit(net_log_schedule_t schedule) {
    if (schedule->m_current_category == NULL) {
        CPE_ERROR(schedule->m_em, "log: commit: no current producer!");
        return;
    }

    log_producer_client * log_client = get_log_producer_client(schedule->m_current_category->m_log_producer, NULL);
    if (log_client == NULL) {
        CPE_ERROR(schedule->m_em, "log: commit: get client fail!");
        return;
    }

    log_producer_result r =
        log_producer_client_add_log_with_len(
            log_client, schedule->m_kv_count, schedule->m_keys, schedule->m_keys_len, schedule->m_values, schedule->m_values_len);
    if (r != LOG_PRODUCER_OK) {
        CPE_ERROR(schedule->m_em, "log: commit fail, rv=%d", r);
    }
}

mem_buffer_t net_log_schedule_tmp_buffer(net_log_schedule_t schedule) {
    return net_schedule_tmp_buffer(schedule->m_net_schedule);
}
