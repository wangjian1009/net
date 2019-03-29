#include "cpe/pal/pal_string.h"
#include "log_producer_manager.h"
#include "net_log_flusher_i.h"
#include "md5.h"
#include "sds.h"

#define LOG_PRODUCER_FLUSH_INTERVAL_MS 100


#define MAX_LOGGROUP_QUEUE_SIZE 1024
#define MIN_LOGGROUP_QUEUE_SIZE 32

#define MAX_MANAGER_FLUSH_COUNT 100  // 10MS * 100
#define MAX_SENDER_FLUSH_COUNT 100 // 10ms * 100

void * log_producer_send_thread(void * param);

char * _get_pack_id(const char * configName, const char * ip)
{
    unsigned char md5Buf[16];
    mbedtls_md5((const unsigned char *)configName, strlen(configName), md5Buf);
    int loop = 0;
    char * val = (char *)malloc(sizeof(char) * 32);
    memset(val, 0, sizeof(char) * 32);
    for(; loop < 8; ++loop)
    {
        unsigned char a = ((md5Buf[loop])>>4) & 0xF, b = (md5Buf[loop]) & 0xF;
        val[loop<<1] = a > 9 ? (a - 10 + 'A') : (a + '0');
        val[(loop<<1)|1] = b > 9 ? (b - 10 + 'A') : (b + '0');
    }
    return val;
}

/* void _try_flush_loggroup(log_producer_manager * producer_manager) */
/* { */
/*     net_log_schedule_t schedule = producer_manager->m_category->m_schedule; */

/*     int32_t now_time = time(NULL); */

/*     if (producer_manager->builder != NULL && now_time - producer_manager->firstLogTime > producer_manager->producer_config->packageTimeoutInMS / 1000) */
/*     { */
/*         log_group_builder * builder = producer_manager->builder; */
/*         producer_manager->builder = NULL; */

/*         size_t loggroup_size = builder->loggroup_size; */
/*         int rst = log_queue_push(producer_manager->loggroup_queue, builder); */
/*         CPE_INFO(schedule->m_em, "try push loggroup to flusher, size : %d, status : %d", (int)loggroup_size, rst); */
/*         if (rst != 0) */
/*         { */
/*             CPE_ERROR(schedule->m_em, "try push loggroup to flusher failed, force drop this log group, error code : %d", rst); */
/*             log_group_destroy(builder); */
/*         } */
/*         else */
/*         { */
/*             producer_manager->totalBufferSize += loggroup_size; */
/*             COND_SIGNAL(producer_manager->triger_cond); */
/*         } */
/*     } */
/*     else */
/*     { */
/*         CS_LEAVE(producer_manager->lock); */
/*     } */
/* } */

log_producer_manager * create_log_producer_manager(net_log_category_t category)
{
    CPE_INFO(category->m_schedule->m_em, "create log producer manager : %s", category->m_name);
    
    log_producer_manager * producer_manager = (log_producer_manager *)malloc(sizeof(log_producer_manager));
    memset(producer_manager, 0, sizeof(log_producer_manager));

    producer_manager->m_category = category;
    
    int32_t base_queue_size = category->m_producer_config->maxBufferBytes / (category->m_producer_config->logBytesPerPackage + 1) + 10;
    if (base_queue_size < MIN_LOGGROUP_QUEUE_SIZE)
    {
        base_queue_size = MIN_LOGGROUP_QUEUE_SIZE;
    }
    else if (base_queue_size > MAX_LOGGROUP_QUEUE_SIZE)
    {
        base_queue_size = MAX_LOGGROUP_QUEUE_SIZE;
    }

    if (category->m_producer_config->source != NULL)
    {
        producer_manager->source = sdsnew(category->m_producer_config->source);
    }
    else
    {
        producer_manager->source = sdsnew("undefined");
    }


    producer_manager->pack_prefix = _get_pack_id(category->m_name, producer_manager->source);
    if (producer_manager->pack_prefix == NULL)
    {
        producer_manager->pack_prefix = (char *)malloc(32);
        srand(time(NULL));
        int i = 0;
        for (i = 0; i < 16; ++i)
        {
            producer_manager->pack_prefix[i] = rand() % 10 + '0';
        }
        producer_manager->pack_prefix[i] = '\0';
    }
    return producer_manager;
}


/* void _push_last_loggroup(log_producer_manager * manager) { */
/*     net_log_schedule_t schedule = manager->m_category->m_schedule; */
   
/*     log_group_builder * builder = manager->builder; */
/*     manager->builder = NULL; */
/*     if (builder != NULL) */
/*     { */
/*         size_t loggroup_size = builder->loggroup_size; */
/*         if (schedule->m_debug) { */
/*             CPE_INFO( */
/*                 schedule->m_em, "try push loggroup to flusher, size : %d, log size %d", */
/*                 (int)builder->loggroup_size, (int)builder->grp->logs.now_buffer_len); */
/*         } */
/*         int32_t status = log_queue_push(manager->loggroup_queue, builder); */
/*         if (status != 0) { */
/*             CPE_ERROR(schedule->m_em, "try push loggroup to flusher failed, force drop this log group, error code : %d", status); */
/*             log_group_destroy(builder); */
/*         } */
/*         else */
/*         { */
/*             manager->totalBufferSize += loggroup_size; */
/*         } */
/*     } */
/* } */

void destroy_log_producer_manager_tail(log_producer_manager * manager)
{
    net_log_schedule_t schedule = manager->m_category->m_schedule;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "delete producer manager tail");
    }
    
    if (manager->pack_prefix != NULL)
    {
        free(manager->pack_prefix);
    }
    sdsfree(manager->source);
    free(manager);
}

void destroy_log_producer_manager(log_producer_manager * manager) {
    net_log_schedule_t schedule = manager->m_category->m_schedule;

    // when destroy instance, flush last loggroup
    //_push_last_loggroup(manager);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "flush out producer loggroup begin");
    }
    
    int32_t total_wait_count = manager->m_category->m_producer_config->destroyFlusherWaitTimeoutSec > 0 ? manager->m_category->m_producer_config->destroyFlusherWaitTimeoutSec * 100 : MAX_MANAGER_FLUSH_COUNT;
    total_wait_count += manager->m_category->m_producer_config->destroySenderWaitTimeoutSec > 0
        ? manager->m_category->m_producer_config->destroySenderWaitTimeoutSec * 100
        : MAX_SENDER_FLUSH_COUNT;

    //usleep(10 * 1000);
    int waitCount = 0;
    /* while (log_queue_size(manager->loggroup_queue) > 0 || */
    /*         manager->send_param_queue_write - manager->send_param_queue_read > 0 || */
    /*         (manager->sender_data_queue != NULL && log_queue_size(manager->sender_data_queue) > 0) ) */
    /* { */
    /*     usleep(10 * 1000); */
    /*     if (++waitCount == total_wait_count) */
    /*     { */
    /*         break; */
    /*     } */
    /* } */
    /* if (waitCount == total_wait_count) */
    /* { */
    /*     CPE_ERROR( */
    /*         schedule->m_em, "try flush out producer loggroup error, force exit, now loggroup %d", */
    /*         (int)(log_queue_size(manager->loggroup_queue))); */
    /* } */
    /* else */
    /* { */
    /*     if (schedule->m_debug) { */
    /*         CPE_INFO(schedule->m_em, "flush out producer loggroup success"); */
    /*     } */
    /* } */
    //manager->shutdown = 1;

    // destroy root resources
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "join flush thread begin");
    }

    destroy_log_producer_manager_tail(manager);
}
