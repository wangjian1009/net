#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_trans_task_i.h"
#include "net_trans_host_i.h"
#include "net_trans_http_endpoint_i.h"

static net_address_t net_trans_task_parse_address(
    net_trans_manage_t mgr, const char * url, uint8_t * is_https, const char * * relative_url);

net_trans_task_t
net_trans_task_create(net_trans_manage_t mgr, const char *method, const char * uri) {
    net_address_t address = NULL;
    net_trans_host_t host = NULL;
    net_trans_http_endpoint_t http_ep = NULL;
    net_trans_task_t task = NULL;

    uint8_t is_https;
    const char * relative_url;
    address = net_trans_task_parse_address(mgr, uri, &is_https, &relative_url);
    if (address == NULL) return NULL;

    host = net_trans_host_check_create(mgr, address);
    if (host == NULL) goto CREATED_ERROR;
    net_address_free(address);
    address = NULL;

    http_ep = net_trans_host_alloc_endpoint(host);
    if (http_ep == NULL) goto CREATED_ERROR;

    task = TAILQ_FIRST(&mgr->m_free_tasks);
    if (task) {
        TAILQ_REMOVE(&mgr->m_free_tasks, task, m_next_for_mgr);
    }
    else {
        task = mem_alloc(mgr->m_alloc, sizeof(struct net_trans_task));
        if (task == NULL) {
            CPE_ERROR(mgr->m_em, "trans: task: alloc fail");
            goto CREATED_ERROR;
        }
    }

    return task;

CREATED_ERROR:
    if (http_ep) {
        if (!net_trans_http_endpoint_is_active(http_ep)) {
            net_trans_http_endpoint_free(http_ep);
        }
        http_ep = NULL;
    }
    
    if (host) {
        if (TAILQ_EMPTY(&host->m_endpoints)) {
            net_trans_host_free(host);
        }
        host = NULL;
    }

    if (address) {
        net_address_free(address);
        address = NULL;
    }

    if (task) {
        task->m_ep = (net_trans_http_endpoint_t)mgr;
        TAILQ_INSERT_TAIL(&mgr->m_free_tasks, task, m_next_for_mgr);
        task = NULL;
    }
    
    return NULL;
}

static net_address_t net_trans_task_parse_address(
    net_trans_manage_t mgr, const char * url, uint8_t * is_https, const char * * relative_url)
{
    const char * host_begin;
    if (cpe_str_start_with(url, "http://")) {
        *is_https = 0;
        host_begin = url + 7;
    }
    else if (cpe_str_start_with(url, "https://")) {
        *is_https = 1;
        host_begin = url + 8;
    }
    else {
        CPE_ERROR(mgr->m_em, "trans: url %s not support, no http or https protocol!", url);
        return NULL;
    }

    net_address_t address = NULL;

    const char * host_end = strchr(host_begin, '/');
    if (host_end) {
        char address_buf[128];
        size_t len = host_end - host_begin;
        if (len >= CPE_ARRAY_SIZE(address_buf)) {
            CPE_ERROR(mgr->m_em, "trans: url %s format error, length %d overflow!", url, (int)len);
            return NULL;
        }

        memcpy(address_buf, host_begin, len);
        address_buf[len] = 0;
        address = net_address_create_auto(mgr->m_schedule, address_buf);
        if (address == NULL) {
            CPE_ERROR(mgr->m_em, "trans: url %s format error, address=%s!", url, address_buf);
            return NULL;
        }

        *relative_url = host_end;
    }
    else {
        address = net_address_create_auto(mgr->m_schedule, host_begin);
        if (address == NULL) {
            CPE_ERROR(mgr->m_em, "trans: url %s format error, address=%s!", url, host_begin);
            return NULL;
        }
        
        *relative_url = "/";
    }

    if (net_address_port(address) == 0) {
        net_address_set_port(address, *is_https ? 433 : 80);
    }
    
    return address;
}
