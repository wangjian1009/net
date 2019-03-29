#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/md5.h"
#include "net_watcher.h"
#include "net_log_request.h"
#include "net_log_util.h"
#include "log_producer_sender.h"
#include "log_producer_manager.h"
#include "lz4.h"

#define SEND_TIME_INVALID_FIX

static int net_log_request_send(net_log_request_t request, net_log_request_param_t send_param);

#ifdef SEND_TIME_INVALID_FIX
static void net_log_request_rebuild_time(net_log_schedule_t schedule, lz4_log_buf_t lz4_buf, lz4_log_buf_t * new_lz4_buf);
#endif

net_log_request_t
net_log_request_create(net_log_request_manage_t mgr, net_log_request_param_t send_param) {
    net_log_schedule_t schedule = mgr->m_schedule;
    net_log_request_t request = TAILQ_FIRST(&mgr->m_requests);

    if (request) {
        TAILQ_REMOVE(&mgr->m_free_requests, request, m_next);
    }
    else {
        request = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request));
        if (request == NULL) {
            CPE_ERROR(schedule->m_em, "log: request: alloc fail!");
            return NULL;
        }
    }

    request->m_mgr = mgr;
    request->m_handler = NULL;
    request->m_watcher = NULL;
    request->m_category = send_param->category;

    TAILQ_INSERT_TAIL(&mgr->m_requests, request, m_next);
    
    /*init success*/
    if (net_log_request_send(request, send_param) != 0) {
        net_log_request_free(request);
        return NULL;
    }
    
    return request;
}

void net_log_request_free(net_log_request_t request) {
    net_log_request_manage_t mgr = request->m_mgr;

    if (request->m_watcher) {
        net_watcher_free(request->m_watcher);
        request->m_watcher = NULL;
    }
    
    if (request->m_handler) {
        curl_easy_cleanup(request->m_handler);
        request->m_handler = NULL;
    }
    
    TAILQ_REMOVE(&mgr->m_requests, request, m_next);
    TAILQ_INSERT_TAIL(&mgr->m_free_requests, request, m_next);
}

void net_log_request_real_free(net_log_request_t request) {
    net_log_request_manage_t mgr = request->m_mgr;
    
    TAILQ_REMOVE(&mgr->m_free_requests, request, m_next);
    mem_free(mgr->m_schedule->m_alloc, request);
}

static size_t net_log_request_on_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t totalLen = size * nmemb;
    /* //printf("body  ---->  %d  %s \n", (int) (totalLen, (const char*) ptr); */
    /* sds * buffer = (sds *)stream; */
    /* if (*buffer == NULL) */
    /* { */
    /*     *buffer = sdsnewEmpty(256); */
    /* } */
    /* *buffer = sdscpylen(*buffer, ptr, totalLen); */
    return totalLen;
}

static size_t net_log_request_on_header(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t totalLen = size * nmemb;
    /* //printf("header  ---->  %d  %s \n", (int) (totalLen), (const char*) ptr); */
    /* sds * buffer = (sds *)stream; */
    /* // only copy header start with x-log- */
    /* if (totalLen > 6 && memcmp(ptr, "x-log-", 6) == 0) */
    /* { */
    /*     *buffer = sdscpylen(*buffer, ptr, totalLen); */
    /* } */
    return totalLen;
}

static int net_log_request_send(net_log_request_t request, net_log_request_param_t send_param) {
    net_log_category_t category =  send_param->category;
    log_producer_manager_t producer_manager = category->m_producer_manager;
    net_log_schedule_t schedule = category->m_schedule;
    log_producer_config_t config = category->m_producer_config;
    lz4_log_buf_t buffer = send_param->log_buf;
    
    if (send_param->magic_num != LOG_PRODUCER_SEND_MAGIC_NUM) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: invalid send param, magic num not found, num 0x%x",
            category->m_id, category->m_name, send_param->magic_num);
        return -1;
    }

    assert(buffer);

    if (request->m_handler) {
        curl_easy_cleanup(request->m_handler);
        request->m_handler = NULL;
    }
    
    request->m_handler = curl_easy_init();
    if (request->m_handler == NULL) {
        CPE_ERROR(schedule->m_em, "log: category [%d]%s: curl_easy_init fail", category->m_id, category->m_name);
        return -1;
    }
    curl_easy_setopt(request->m_handler, CURLOPT_PRIVATE, request);

    char buf[512];
    size_t sz;
    
    // url
    snprintf(
        buf, sizeof(buf), "%s://%s.%s/logstores/%s/shards/lb", 
        schedule->m_cfg_using_https ? "https" : "http",
        schedule->m_cfg_project,
        schedule->m_cfg_ep,
        category->m_name);
    curl_easy_setopt(request->m_handler, CURLOPT_URL, buf);
    
    struct curl_slist *connect_to = NULL;
    if (config->remote_address != NULL) {
        // example.com::192.168.1.5:
        snprintf(buf, sizeof(buf), "%s.%s::%s:", schedule->m_cfg_project, schedule->m_cfg_ep, config->remote_address);
        connect_to = curl_slist_append(connect_to, buf);
        curl_easy_setopt(request->m_handler, CURLOPT_CONNECT_TO, connect_to);
    }

    char nowTime[64];
    time_t rawtime = time(0);
    struct tm * timeinfo = gmtime(&rawtime);
    strftime(nowTime, sizeof(nowTime), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);

    char md5Buf[33];
    md5Buf[32] = '\0';
    md5_to_string((const char *)buffer->data, buffer->length, md5Buf);
    
    int lz4Flag = config->compressType == 1;
    
    struct curl_slist * headers = NULL;

    headers = curl_slist_append(headers, "Content-Type:application/x-protobuf");
    headers = curl_slist_append(headers, "x-log-apiversion:0.6.0");
    if (lz4Flag) {
        headers = curl_slist_append(headers, "x-log-compresstype:lz4");
    }

    headers = curl_slist_append(headers, "x-log-signaturemethod:hmac-sha1");

    /**/
    snprintf(buf, sizeof(buf), "Date:%s", nowTime);
    headers = curl_slist_append(headers, buf);

    /**/
    snprintf(buf, sizeof(buf), "Content-MD5:%s", md5Buf);
    headers = curl_slist_append(headers, buf);

    /**/
    snprintf(buf, sizeof(buf), "Content-Length:%d", (int)buffer->length);
    headers = curl_slist_append(headers, buf);

    /**/
    snprintf(buf, sizeof(buf), "x-log-bodyrawsize:%d", (int)buffer->raw_length);
    headers = curl_slist_append(headers, buf);

    /**/
    snprintf(buf, sizeof(buf), "Host:%s.%s", schedule->m_cfg_project, schedule->m_cfg_ep);
    headers = curl_slist_append(headers, buf);

    if (lz4Flag) {
        sz = snprintf(
            buf, sizeof(buf),
            "POST\n%s\napplication/x-protobuf\n%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\nx-log-compresstype:lz4\nx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
            md5Buf, nowTime, (int)buffer->raw_length, category->m_name);
    }
    else {
        sz = snprintf(
            buf, sizeof(buf),
            "POST\n%s\napplication/x-protobuf\n%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\nx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
            md5Buf, nowTime, (int)buffer->raw_length, category->m_name);
    }

    char sha1Buf[65];
    int sha1Len = signature_to_base64(buf, sz, schedule->m_cfg_access_key, strlen(schedule->m_cfg_access_key), sha1Buf);
    sha1Buf[sha1Len] = 0;

    snprintf(buf, sizeof(buf),  "Authorization:LOG %s:%s", schedule->m_cfg_access_id, sha1Buf);
    headers = curl_slist_append(headers, buf);

    curl_easy_setopt(request->m_handler, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(request->m_handler, CURLOPT_POST, 1);

    curl_easy_setopt(request->m_handler, CURLOPT_POSTFIELDS, (void *)buffer->data);
    curl_easy_setopt(request->m_handler, CURLOPT_POSTFIELDSIZE, buffer->length);

    curl_easy_setopt(request->m_handler, CURLOPT_FILETIME, 1);
    curl_easy_setopt(request->m_handler, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(request->m_handler, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(request->m_handler, CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt(request->m_handler, CURLOPT_NETRC, CURL_NETRC_IGNORED);

    curl_easy_setopt(request->m_handler, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(request->m_handler, CURLOPT_USERAGENT, "log-c-lite_0.1.0");

    if (config->netInterface != NULL) {
        curl_easy_setopt(request->m_handler, CURLOPT_INTERFACE, config->netInterface);
    }
    
    curl_easy_setopt(request->m_handler, CURLOPT_TIMEOUT, config->sendTimeoutSec > 0 ? config->sendTimeoutSec : 15);
    
    if (config->connectTimeoutSec > 0) {
        curl_easy_setopt(request->m_handler, CURLOPT_CONNECTTIMEOUT, config->connectTimeoutSec);
    }

    curl_easy_setopt(request->m_handler, CURLOPT_HEADERFUNCTION, net_log_request_on_header);
    curl_easy_setopt(request->m_handler, CURLOPT_HEADERDATA, request);

    curl_easy_setopt(request->m_handler, CURLOPT_WRITEFUNCTION, net_log_request_on_data);
    curl_easy_setopt(request->m_handler, CURLOPT_WRITEDATA, request);

    //curl_easy_setopt(request->m_handler, CURLOPT_VERBOSE, 1); //打印调试信息

    return 0;
}

/*     int32_t sleepMs = log_producer_on_send_done(send_param, rst, &error_info); */

/*     post_log_result_destroy(rst); */

/*     // tmp buffer, free */
/*     if (send_buf != send_param->log_buf) */
/*     { */
/*         free(send_buf); */
/*     } */

/*     if (sleepMs <= 0) */
/*     { */
/*         break; */
/*     } */
/*     int i =0; */
/*     for (i = 0; i < sleepMs; i += SEND_SLEEP_INTERVAL_MS) */
/*     { */
/*         usleep(SEND_SLEEP_INTERVAL_MS * 1000); */
/*         if (producer_manager->shutdown || producer_manager->networkRecover) */
/*         { */
/*             break; */
/*         } */
/*     } */

/*     /\* if (producer_manager->networkRecover) { *\/ */
/*     /\*     producer_manager->networkRecover = 0; *\/ */
/*     /\* } *\/ */

/*     // at last, free all buffer */
/*     /\* free_lz4_log_buf(send_param->log_buf); *\/ */
/*     /\* free(send_param); *\/ */

/*     return NULL; */
/* } */

net_log_request_param_t
create_net_log_request_param(
    net_log_category_t category, lz4_log_buf * log_buf, uint32_t builder_time)
{
    net_log_request_param_t param = (net_log_request_param_t)malloc(sizeof(struct net_log_request_param));
    param->category = category;
    param->log_buf = log_buf;
    param->magic_num = LOG_PRODUCER_SEND_MAGIC_NUM;
    param->builder_time = builder_time;
    return param;
}

void net_log_request_param_free(net_log_request_param_t send_param) {
    free_lz4_log_buf(send_param->log_buf);
    free(send_param);
}

    /* CURLcode res = curl_easy_perform(curl); */
    /* //printf("result : %s \n", curl_easy_strerror(res)); */
    /* long http_code; */
    /* if (res == CURLE_OK) */
    /* { */
    /*     if ((res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)) != CURLE_OK) */
    /*     { */
    /*         printf("get info result : %s \n", curl_easy_strerror(res)); */
    /*         result->statusCode = -2; */
    /*     } else { */
    /*         result->statusCode = http_code; */
    /*     } */
    /* } */
    /* else */
    /* { */
    /*     if (body == NULL) */
    /*     { */
    /*         body = sdsnew(curl_easy_strerror(res)); */
    /*     } */
    /*     else */
    /*     { */
    /*         body = sdscpy(body, curl_easy_strerror(res)); */
    /*     } */
    /*     result->statusCode = -1 * (int)res; */
    /* } */
    /* // header and body 's pointer may be modified in callback (size > 256) */
    /* if (sdslen(header) > 0) */
    /* { */
    /*     result->requestID = header; */
    /* } */
    /* else */
    /* { */
    /*     sdsfree(header); */
    /*     header = NULL; */
    /* } */
    /* // body will be NULL or a error string(net error or request error) */
    /* result->errorMessage = body; */

    /* return result; */

void net_log_request_complete(net_log_request_t request, net_log_request_complete_state_t complete_state) {
    net_log_category_t category = request->m_category;
    assert(category);

    /* net_log_request_send_result_t send_result; */
    /* if (net_log_request_calc_result(net_log_request_send_result_t * result, net_log_schedule_t schedule, net_log_request_t request) { */
/*     log_producer_manager * producer_manager = category->m_producer_manager; */
/*     log_producer_send_result send_result = AosStatusToResult(result); */
/*     net_log_schedule_t schedule = category->m_schedule; */

/*     switch (send_result) */
/*     { */
/*         case LOG_SEND_OK: */
/*             break; */
/*         case LOG_SEND_TIME_ERROR: */
/*             // if no this marco, drop data */
/* #ifdef SEND_TIME_INVALID_FIX */
/*             error_info->last_send_error = LOG_SEND_TIME_ERROR; */
/*             error_info->last_sleep_ms = INVALID_TIME_TRY_INTERVAL; */
/*             return error_info->last_sleep_ms; */
/* #else */
/*             break; */
/* #endif */
/*         case LOG_SEND_QUOTA_EXCEED: */
/*             if (error_info->last_send_error != LOG_SEND_QUOTA_EXCEED) */
/*             { */
/*                 error_info->last_send_error = LOG_SEND_QUOTA_EXCEED; */
/*                 error_info->last_sleep_ms = BASE_QUOTA_ERROR_SLEEP_MS; */
/*                 error_info->first_error_time = time(NULL); */
/*             } */
/*             else */
/*             { */
/*                 if (error_info->last_sleep_ms < MAX_QUOTA_ERROR_SLEEP_MS) */
/*                 { */
/*                     error_info->last_sleep_ms *= 2; */
/*                 } */

/* #ifndef SEND_TIME_INVALID_FIX */
/*                 // only drop data when SEND_TIME_INVALID_FIX not defined */
/*                 if (time(NULL) - error_info->first_error_time > DROP_FAIL_DATA_TIME_SECOND) */
/*                 { */
/*                     break; */
/*                 } */
/* #endif */
/*             } */
/*             if (schedule->m_debug) { */
/*                 CPE_INFO(schedule->m_em, "send quota error, project : %s, logstore : %s, buffer len : %d, raw len : %d, code : %d, error msg : %s", */
/*                          schedule->m_cfg_project, */
/*                          category->m_name, */
/*                          (int)send_param->log_buf->length, */
/*                          (int)send_param->log_buf->raw_length, */
/*                          result->statusCode, */
/*                          result->errorMessage); */
/*             } */
/*             return error_info->last_sleep_ms; */
/*         case LOG_SEND_SERVER_ERROR : */
/*         case LOG_SEND_NETWORK_ERROR: */
/*             if (error_info->last_send_error != LOG_SEND_NETWORK_ERROR) */
/*             { */
/*                 error_info->last_send_error = LOG_SEND_NETWORK_ERROR; */
/*                 error_info->last_sleep_ms = BASE_NETWORK_ERROR_SLEEP_MS; */
/*                 error_info->first_error_time = time(NULL); */
/*             } */
/*             else */
/*             { */
/*                 if (error_info->last_sleep_ms < MAX_NETWORK_ERROR_SLEEP_MS) */
/*                 { */
/*                     error_info->last_sleep_ms *= 2; */
/*                 } */
/* #ifndef SEND_TIME_INVALID_FIX */
/*                 // only drop data when SEND_TIME_INVALID_FIX not defined */
/*                 if (time(NULL) - error_info->first_error_time > DROP_FAIL_DATA_TIME_SECOND) */
/*                 { */
/*                     break; */
/*                 } */
/* #endif */
/*             } */
/*             if (schedule->m_debug) { */
/*                 CPE_INFO( */
/*                     schedule->m_em, "send network error, project : %s, logstore : %s, buffer len : %d, raw len : %d, code : %d, error msg : %s", */
/*                     schedule->m_cfg_project, */
/*                     category->m_name, */
/*                     (int)send_param->log_buf->length, */
/*                     (int)send_param->log_buf->raw_length, */
/*                     result->statusCode, */
/*                     result->errorMessage); */
/*             } */
/*             return error_info->last_sleep_ms; */
/*         default: */
/*             // discard data */
/*             break; */

/*     } */

/*     producer_manager->totalBufferSize -= send_param->log_buf->length; */
/*     if (send_result == LOG_SEND_OK) */
/*     { */
/*         if (schedule->m_debug) { */
/*             CPE_INFO( */
/*                 schedule->m_em, "send success, project : %s, logstore : %s, buffer len : %d, raw len : %d, total buffer : %d, code : %d, error msg : %s", */
/*                 schedule->m_cfg_project, */
/*                 category->m_name, */
/*                 (int)send_param->log_buf->length, */
/*                 (int)send_param->log_buf->raw_length, */
/*                 (int)producer_manager->totalBufferSize, */
/*                 result->statusCode, */
/*                 result->errorMessage); */
/*         } */
/*     } */
/*     else */
/*     { */
/*         CPE_ERROR( */
/*             schedule->m_em, "send fail, discard data, project : %s, logstore : %s, buffer len : %d, raw len : %d, total buffer : %d, code : %d, error msg : %s", */
/*             schedule->m_cfg_project, */
/*             category->m_name, */
/*             (int)send_param->log_buf->length, */
/*             (int)send_param->log_buf->raw_length, */
/*             (int)producer_manager->totalBufferSize, */
/*             result->statusCode, */
/*             result->errorMessage); */
/*     } */
}

static int net_log_request_calc_result(net_log_request_send_result_t * result, net_log_schedule_t schedule, net_log_request_t request) {
    long http_code = 0;    
    CURLcode res = curl_easy_getinfo(request->m_handler, CURLINFO_RESPONSE_CODE, &http_code);
    if (res != CURLE_OK) {
        CPE_ERROR(schedule->m_em, "log: request: get curl response code fail: %s", curl_easy_strerror(res));
        return -1;
    }
    
    if (http_code / 100 == 2) {
        *result = net_log_request_send_ok;
    }
    else if (http_code <= 0) {
        *result = net_log_request_send_network_error;
    }
    /* else if (http_code >= 500 || result->requestID == NULL) */
    /* { */
    /*     return LOG_SEND_SERVER_ERROR; */
    /* } */
    /* if (http_code == 403) */
    /* { */
    /*     return LOG_SEND_QUOTA_EXCEED; */
    /* } */
    /* if (http_code == 401 || http_code == 404) */
    /* { */
    /*     return LOG_SEND_UNAUTHORIZED; */
    /* } */
    /* if (result->errorMessage != NULL && strstr(result->errorMessage, LOGE_TIME_EXPIRED) != NULL) */
    /* { */
    /*     return LOG_SEND_TIME_ERROR; */
    /* } */

    /* return LOG_SEND_DISCARD_ERROR; */

    return 0;
}

#ifdef SEND_TIME_INVALID_FIX

static void net_log_request__rebuild_time(net_log_schedule_t schedule, lz4_log_buf * lz4_buf, lz4_log_buf ** new_lz4_buf) {
    CPE_INFO(schedule->m_em, "rebuild log.");
    char * buf = (char *)malloc(lz4_buf->raw_length);
    if (LZ4_decompress_safe((const char* )lz4_buf->data, buf, lz4_buf->length, lz4_buf->raw_length) <= 0) {
        free(buf);
        CPE_ERROR(schedule->m_em, "LZ4_decompress_safe error");
        return;
    }

    uint32_t nowTime = time(NULL);
    fix_log_group_time(buf, lz4_buf->raw_length, nowTime);

    int compress_bound = LZ4_compressBound(lz4_buf->raw_length);
    char *compress_data = (char *)malloc(compress_bound);
    int compressed_size = LZ4_compress_default((char *)buf, compress_data, lz4_buf->raw_length, compress_bound);
    if(compressed_size <= 0) {
        CPE_ERROR(schedule->m_em, "LZ4_compress_default error");
        free(buf);
        free(compress_data);
        return;
    }

    *new_lz4_buf = (lz4_log_buf*)malloc(sizeof(lz4_log_buf) + compressed_size);
    (*new_lz4_buf)->length = compressed_size;
    (*new_lz4_buf)->raw_length = lz4_buf->raw_length;
    memcpy((*new_lz4_buf)->data, compress_data, compressed_size);
    free(buf);
    free(compress_data);
    return;
}

#endif
