//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_client.h"
#include "log_producer_manager.h"
#include "inner_log.h"
#include "log_api.h"
#include <stdarg.h>
#include <string.h>


extern log_queue * g_sender_data_queue;
extern THREAD * g_send_threads;
extern int32_t g_send_thread_count;
extern volatile uint8_t g_send_thread_destroy;

extern void * log_producer_send_thread_global(void * param);
extern void log_producer_send_thread_global_inner(log_producer_send_param * send_param);
extern void destroy_log_producer_manager_tail(log_producer_manager * manager);

typedef struct _producer_client_private {

    log_producer_manager * producer_manager;
    log_producer_config * producer_config;
}producer_client_private ;

void log_producer_env_destroy()
{
    // destroy global send threads
    if (g_send_threads != NULL) {
        g_send_thread_destroy = 1;
        aos_info_log("join global sender thread pool begin, thread count : %d", g_send_thread_count);
        int32_t threadId = 0;
        for (; threadId < g_send_thread_count; ++threadId)
        {
            THREAD_JOIN(g_send_threads[threadId]);
            aos_info_log("join one global sender thread pool done, thread id : %d", threadId);
        }
        free(g_send_threads);
        aos_info_log("flush out global sender queue begin");
        while (log_queue_size(g_sender_data_queue) > 0)
        {
            // @note : we must process all data in queue, otherwise this will cause memory leak
            log_producer_send_param * send_param = (log_producer_send_param *)log_queue_trypop(g_sender_data_queue);
            log_producer_send_thread_global_inner(send_param);
        }
        aos_info_log("flush out global sender queue success");
        log_queue_destroy(g_sender_data_queue);
        g_sender_data_queue = NULL;
        g_send_thread_destroy = 0;
        g_send_thread_count = 0;
        g_send_threads = NULL;
        aos_info_log("join global sender thread pool success");
    }
}

log_producer_client * create_log_client(log_producer_config * config, on_log_producer_send_done_function send_done_function)
{
    if (!log_producer_config_is_valid(config))
    {
        return NULL;
    }
    log_producer_client * producer_client = (log_producer_client *)malloc(sizeof(log_producer_client));
    producer_client_private * client_private = (producer_client_private *)malloc(sizeof(producer_client_private));
    producer_client->private_data = client_private;
    client_private->producer_config = config;
    client_private->producer_manager = create_log_producer_manager(config);
    client_private->producer_manager->send_done_function = send_done_function;

    if(client_private->producer_manager == NULL)
    {
        // free
        free(producer_client);
        free(client_private);
        return NULL;
    }
    aos_debug_log("create producer client success, config : %s", config->logstore);
    producer_client->valid_flag = 1;
    return producer_client;
}


void destroy_log_client(log_producer_client * client)
{
    client->valid_flag = 0;
    producer_client_private * client_private = (producer_client_private *)client->private_data;
    destroy_log_producer_manager(client_private->producer_manager);

    free(client_private);
    free(client);
}

void log_producer_client_network_recover(log_producer_client * client)
{
    if (client == NULL)
    {
        return;
    }
    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    manager->networkRecover = 1;
}

log_producer_result log_producer_client_add_log(log_producer_client * client, int32_t kv_count, ...)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }
    va_list argp;
    va_start(argp, kv_count);

    int32_t pairs = kv_count / 2;

    char ** keys = (char **)malloc(pairs * sizeof(char *));
    char ** values = (char **)malloc(pairs * sizeof(char *));
    size_t * key_lens = (size_t *)malloc(pairs * sizeof(size_t));
    size_t * val_lens = (size_t *)malloc(pairs * sizeof(size_t));

    int32_t i = 0;
    for (; i < pairs; ++i)
    {
        const char * key = va_arg(argp, const char *);
        const char * value = va_arg(argp, const char *);
        keys[i] = (char *)key;
        values[i] = (char *)value;
        key_lens[i] = strlen(key);
        val_lens[i] = strlen(value);
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;

    log_producer_result rst = log_producer_manager_add_log(manager, pairs, keys, key_lens, values, val_lens);
    free(keys);
    free(values);
    free(key_lens);
    free(val_lens);
    return rst;
}

log_producer_result log_producer_client_add_log_with_len(log_producer_client * client, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;

    return log_producer_manager_add_log(manager, pair_count, keys, key_lens, values, val_lens);
}


log_producer_result log_producer_client_add_raw_log_buffer(log_producer_client * client, size_t log_bytes, size_t compressed_bytes, const unsigned char * raw_buffer)
{
    if (client == NULL || !client->valid_flag || raw_buffer == NULL)
    {
        return LOG_PRODUCER_INVALID;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;

    return log_producer_manager_send_raw_buffer(manager, log_bytes, compressed_bytes, raw_buffer);
}

log_producer_result log_producer_global_send_thread_init(int32_t log_global_send_thread_count, int32_t log_global_send_queue_size)
{
    if (log_global_send_thread_count <= 0  || log_global_send_queue_size <= 0 || g_send_threads != NULL) {
        return LOG_PRODUCER_INVALID;
    }
    g_send_thread_count = log_global_send_thread_count;
    g_send_threads = (THREAD *)malloc(sizeof(THREAD) * g_send_thread_count);

    g_sender_data_queue = log_queue_create(log_global_send_queue_size);
    g_send_thread_destroy = 0;
    int32_t threadId = 0;
    for (; threadId < g_send_thread_count; ++threadId)
    {
        THREAD_INIT(g_send_threads[threadId], log_producer_send_thread_global, g_sender_data_queue);
    }
    return LOG_PRODUCER_OK;
}
