#include <assert.h>
#include "cpe/utils/md5.h"
#include "net_log_request.h"
#include "log_producer_sender.h"
#include "log_producer_manager.h"
#include "log_util.h"

static int net_log_request_send(net_log_request_t request, log_producer_send_param_t send_param);

net_log_request_t
net_log_request_create(net_log_request_manage_t mgr, log_producer_send_param_t send_param) {
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

static int net_log_request_prepare(
    net_log_schedule_t schedule, net_log_category_t category, net_log_request_t request,
    lz4_log_buf * buffer, log_producer_config * config)
{
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

static int net_log_request_send(net_log_request_t request, log_producer_send_param_t send_param) {
    log_producer_manager_t producer_manager = (log_producer_manager_t)send_param->producer_manager;
    net_log_category_t category =  producer_manager->m_category;
    net_log_schedule_t schedule = category->m_schedule;

    if (send_param->magic_num != LOG_PRODUCER_SEND_MAGIC_NUM) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: invalid send param, magic num not found, num 0x%x",
            category->m_id, category->m_name, send_param->magic_num);
        return -1;
    }

    assert(send_param->log_buf);

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

    int rv = net_log_request_prepare(
        schedule, category, request,
        send_param->log_buf, send_param->producer_config);

    return rv;
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
