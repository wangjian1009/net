//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_sender.h"
#include "lz4.h"
#include "sds.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char* LOGE_SERVER_BUSY = "ServerBusy";
const char* LOGE_INTERNAL_SERVER_ERROR = "InternalServerError";
const char* LOGE_UNAUTHORIZED = "Unauthorized";
const char* LOGE_WRITE_QUOTA_EXCEED = "WriteQuotaExceed";
const char* LOGE_SHARD_WRITE_QUOTA_EXCEED = "ShardWriteQuotaExceed";
const char* LOGE_TIME_EXPIRED = "RequestTimeExpired";

#define SEND_SLEEP_INTERVAL_MS 100
#define MAX_NETWORK_ERROR_SLEEP_MS 3600000
#define BASE_NETWORK_ERROR_SLEEP_MS 1000
#define MAX_QUOTA_ERROR_SLEEP_MS 60000
#define BASE_QUOTA_ERROR_SLEEP_MS 3000
#define INVALID_TIME_TRY_INTERVAL 3000

#define DROP_FAIL_DATA_TIME_SECOND (3600 * 6)

#define SEND_TIME_INVALID_FIX

extern volatile uint8_t g_send_thread_destroy;

typedef struct _send_error_info
{
    log_producer_send_result last_send_error;
    int32_t last_sleep_ms;
    int32_t first_error_time;
}send_error_info;

//int32_t log_producer_on_send_done(net_log_request_param_t send_param, post_log_result * result, send_error_info * error_info);

void * log_producer_send_fun(void * param)
{
/*     net_log_request_param * send_param = (net_log_request_param *)param; */
/*     log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager; */
/*     net_log_category_t category = producer_manager->m_category; */
/*     net_log_schedule_t schedule = category->m_schedule; */

/*     log_producer_config * config = send_param->producer_config; */

/*     send_error_info error_info; */
/*     memset(&error_info, 0, sizeof(error_info)); */

/*     do */
/*     { */
/*         if (producer_manager->shutdown) */
/*         { */
/*             CPE_ERROR(schedule->m_em, "send fail but shutdown signal received, force exit"); */
/*             if (producer_manager->send_done_function != NULL) */
/*             { */
/*                 producer_manager->send_done_function(category->m_name, LOG_PRODUCER_SEND_EXIT_BUFFERED, send_param->log_buf->raw_length, send_param->log_buf->length, */
/*                                                      NULL, "producer is being destroyed, producer has no time to send this buffer out", send_param->log_buf->data); */
/*             } */
/*             break; */
/*         } */
/*         lz4_log_buf * send_buf = send_param->log_buf; */
/* #ifdef SEND_TIME_INVALID_FIX */
/*         uint32_t nowTime = time(NULL); */
/*         if (nowTime - send_param->builder_time > 600 || send_param->builder_time > nowTime || error_info.last_send_error == LOG_SEND_TIME_ERROR) */
/*         { */
/*             _rebuild_time(schedule, send_param->log_buf, &send_buf); */
/*             send_param->builder_time = nowTime; */
/*         } */
/* #endif */

/*         sds accessKeyId = NULL; */
/*         sds accessKey = NULL; */
/*         sds stsToken = NULL; */
/*         log_producer_config_get_security(config, &accessKeyId, &accessKey, &stsToken); */

/*         post_log_result * rst = post_logs_from_lz4buf(config->endpoint, accessKeyId, */
/*                                                       accessKey, stsToken, */
/*                                                       config->project, config->logstore, */
/*                                                       send_buf, &option); */
/*         sdsfree(accessKeyId); */
/*         sdsfree(accessKey); */
/*         sdsfree(stsToken); */

/*         int32_t sleepMs = log_producer_on_send_done(send_param, rst, &error_info); */

/*         post_log_result_destroy(rst); */

/*         // tmp buffer, free */
/*         if (send_buf != send_param->log_buf) */
/*         { */
/*             free(send_buf); */
/*         } */

/*         if (sleepMs <= 0) */
/*         { */
/*             break; */
/*         } */
/*         int i =0; */
/*         for (i = 0; i < sleepMs; i += SEND_SLEEP_INTERVAL_MS) */
/*         { */
/*             usleep(SEND_SLEEP_INTERVAL_MS * 1000); */
/*             if (producer_manager->shutdown || producer_manager->networkRecover) */
/*             { */
/*                 break; */
/*             } */
/*         } */

/*         if (producer_manager->networkRecover) */
/*         { */
/*             producer_manager->networkRecover = 0; */
/*         } */

/*     }while(1); */

/*     // at last, free all buffer */
/*     free_lz4_log_buf(send_param->log_buf); */
/*     free(send_param); */

    return NULL;
}


log_producer_result log_producer_send_data(net_log_request_param_t send_param)
{
    log_producer_send_fun(send_param);
    return LOG_PRODUCER_OK;
}
