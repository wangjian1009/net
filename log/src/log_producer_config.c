#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdlib.h"
#include "log_producer_config.h"
#include "sds.h"

static void _set_default_producer_config(log_producer_config * pConfig)
{
    pConfig->logBytesPerPackage = 3 * 1024 * 1024;
    pConfig->logCountPerPackage = 2048;
    pConfig->packageTimeoutInMS = 3000;
    pConfig->maxBufferBytes = 64 * 1024 * 1024;

    pConfig->connectTimeoutSec = 10;
    pConfig->sendTimeoutSec = 15;
    pConfig->destroySenderWaitTimeoutSec = 1;
    pConfig->destroyFlusherWaitTimeoutSec = 1;
    pConfig->compressType = 1;
}


static void _copy_config_string(const char * value, sds * src_value)
{
    if (value == NULL || src_value == NULL)
    {
        return;
    }
    size_t strLen = strlen(value);
    if (*src_value == NULL)
    {
        *src_value = sdsnewEmpty(strLen);
    }
    *src_value = sdscpylen(*src_value, value, strLen);
}


log_producer_config * create_log_producer_config(net_log_category_t category)
{
    log_producer_config* pConfig = (log_producer_config*)malloc(sizeof(log_producer_config));
    memset(pConfig, 0, sizeof(log_producer_config));
    pConfig->m_category = category;
    _set_default_producer_config(pConfig);
    return pConfig;
}


void destroy_log_producer_config(log_producer_config * pConfig)
{
    if (pConfig->topic != NULL)
    {
        sdsfree(pConfig->topic);
    }
    if (pConfig->tagCount > 0 && pConfig->tags != NULL)
    {
        int i = 0;
        for (; i < pConfig->tagCount; ++i)
        {
            sdsfree(pConfig->tags[i].key);
            sdsfree(pConfig->tags[i].value);
        }
        free(pConfig->tags);
    }
    free(pConfig);
}

#ifdef LOG_PRODUCER_DEBUG
void log_producer_config_print(log_producer_config * pConfig, FILE * file)
{
    fprintf(file, "endpoint : %s\n", pConfig->endpoint);
    fprintf(file,"project : %s\n", pConfig->project);
    fprintf(file,"logstore : %s\n", pConfig->logstore);
    fprintf(file,"accessKeyId : %s\n", pConfig->accessKeyId);
    fprintf(file,"accessKey : %s\n", pConfig->accessKey);
    fprintf(file,"configName : %s\n", pConfig->configName);
    fprintf(file,"topic : %s\n", pConfig->topic);
    fprintf(file,"logLevel : %d\n", pConfig->logLevel);

    fprintf(file,"packageTimeoutInMS : %d\n", pConfig->packageTimeoutInMS);
    fprintf(file, "logCountPerPackage : %d\n", pConfig->logCountPerPackage);
    fprintf(file, "logBytesPerPackage : %d\n", pConfig->logBytesPerPackage);
    fprintf(file, "maxBufferBytes : %d\n", pConfig->maxBufferBytes);


    fprintf(file, "tags: \n");
    int32_t i = 0;
    for (i = 0; i < pConfig->tagCount; ++i)
    {
        fprintf(file, "tag key : %s, value : %s \n", pConfig->tags[i].key, pConfig->tags[i].value);
    }

}
#endif

void log_producer_config_set_packet_timeout(log_producer_config * config, int32_t time_out_ms)
{
    if (config == NULL || time_out_ms < 0)
    {
        return;
    }
    config->packageTimeoutInMS = time_out_ms;
}
void log_producer_config_set_packet_log_count(log_producer_config * config, int32_t log_count)
{
    if (config == NULL || log_count < 0)
    {
        return;
    }
    config->logCountPerPackage = log_count;
}
void log_producer_config_set_packet_log_bytes(log_producer_config * config, int32_t log_bytes)
{
    if (config == NULL || log_bytes < 0)
    {
        return;
    }
    config->logBytesPerPackage = log_bytes;
}
void log_producer_config_set_max_buffer_limit(log_producer_config * config, int64_t max_buffer_bytes)
{
    if (config == NULL || max_buffer_bytes < 0)
    {
        return;
    }
    config->maxBufferBytes = max_buffer_bytes;
}

void log_producer_config_set_connect_timeout_sec(log_producer_config * config, int32_t connect_timeout_sec)
{
    if (config == NULL || connect_timeout_sec <= 0)
    {
        return;
    }
    config->connectTimeoutSec = connect_timeout_sec;
}


void log_producer_config_set_send_timeout_sec(log_producer_config * config, int32_t send_timeout_sec)
{
    if (config == NULL || send_timeout_sec <= 0)
    {
        return;
    }
    config->sendTimeoutSec = send_timeout_sec;
}

void log_producer_config_set_destroy_flusher_wait_sec(log_producer_config * config, int32_t destroy_flusher_wait_sec)
{
    if (config == NULL || destroy_flusher_wait_sec <= 0)
    {
        return;
    }
    config->destroyFlusherWaitTimeoutSec = destroy_flusher_wait_sec;
}

void log_producer_config_set_destroy_sender_wait_sec(log_producer_config * config, int32_t destroy_sender_wait_sec)
{
    if (config == NULL || destroy_sender_wait_sec <= 0)
    {
        return;
    }
    config->destroySenderWaitTimeoutSec = destroy_sender_wait_sec;
}

void log_producer_config_set_compress_type(log_producer_config * config, int32_t compress_type)
{
    if (config == NULL || compress_type < 0 || compress_type > 1)
    {
        return;
    }
    config->compressType = compress_type;
}

void log_producer_config_add_tag(log_producer_config * pConfig, const char * key, const char * value)
{
    if(key == NULL || value == NULL)
    {
        return;
    }
    ++pConfig->tagCount;
    if (pConfig->tags == NULL || pConfig->tagCount > pConfig->tagAllocSize)
    {
        if(pConfig->tagAllocSize == 0)
        {
            pConfig->tagAllocSize = 4;
        }
        else
        {
            pConfig->tagAllocSize *= 2;
        }
        log_producer_config_tag * tagArray = (log_producer_config_tag *)malloc(sizeof(log_producer_config_tag) * pConfig->tagAllocSize);
        if (pConfig->tags != NULL)
        {
            memcpy(tagArray, pConfig->tags, sizeof(log_producer_config_tag) * (pConfig->tagCount - 1));
            free(pConfig->tags);
        }
        pConfig->tags = tagArray;
    }
    int32_t tagIndex = pConfig->tagCount - 1;
    pConfig->tags[tagIndex].key = sdsnew(key);
    pConfig->tags[tagIndex].value = sdsnew(value);

}

void log_producer_config_set_topic(log_producer_config * config, const char * topic)
{
    _copy_config_string(topic, &config->topic);
}
