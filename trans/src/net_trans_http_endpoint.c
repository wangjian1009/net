#include "assert.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_strings.h"
#include "net_endpoint.h"
#include "net_http_endpoint.h"
#include "net_http_protocol.h"
#include "net_trans_http_endpoint_i.h"
#include "net_trans_host_i.h"
#include "net_trans_task_i.h"

net_trans_http_endpoint_t
net_trans_http_endpoint_create(net_trans_host_t host) {
    net_trans_manage_t mgr = host->m_mgr;
    
    net_http_endpoint_t http_endpoint =
        net_http_endpoint_create(
            mgr->m_driver, net_http_protocol_from_data(mgr->m_http_protocol));
    if (http_endpoint == NULL) {
        CPE_ERROR(mgr->m_em, "trans: create http-endpoint fail!");
        return NULL;
    }

    net_trans_http_endpoint_t trans_http = net_http_endpoint_data(http_endpoint);

    if (net_endpoint_set_remote_address(net_http_endpoint_net_ep(http_endpoint), host->m_address, 0) != 0) {
        CPE_ERROR(mgr->m_em, "trans: create http-endpoint: set remote address fail!");
        net_http_endpoint_free(http_endpoint);
        return NULL;
    }
    
    trans_http->m_host = host;
    host->m_endpoint_count++;
    TAILQ_INSERT_TAIL(&host->m_endpoints, trans_http, m_next);
    
    return trans_http;
}

void net_trans_http_endpoint_free(net_trans_http_endpoint_t trans_http) {
    net_http_endpoint_free(net_http_endpoint_from_data(trans_http));
}

int net_trans_http_endpoint_init(net_http_endpoint_t http_endpoint) {
    net_trans_http_endpoint_t trans_http = net_http_endpoint_data(http_endpoint);
    trans_http->m_host = NULL;
    trans_http->m_task_count = 0;
    TAILQ_INIT(&trans_http->m_tasks);

    net_http_endpoint_set_reconnect_span_ms(http_endpoint, 0);
    
    return 0;
}

void net_trans_http_endpoint_fini(net_http_endpoint_t http_endpoint) {
    net_trans_http_endpoint_t trans_http = net_http_endpoint_data(http_endpoint);

    while(!TAILQ_EMPTY(&trans_http->m_tasks)) {
        net_trans_task_free(TAILQ_FIRST(&trans_http->m_tasks));
    }
    assert(trans_http->m_task_count == 0);

    if (trans_http->m_host) {
        assert(trans_http->m_host->m_endpoint_count > 0);
        trans_http->m_host->m_endpoint_count--;
        TAILQ_REMOVE(&trans_http->m_host->m_endpoints, trans_http, m_next);
        trans_http->m_host = NULL;
    }
}

int net_trans_http_endpoint_on_state_change(net_http_endpoint_t http_endpoint, net_http_state_t from_state) {
    switch(net_http_endpoint_state(http_endpoint)) {
    case net_http_state_disable:
        return -1;
    case net_http_state_error:
        return -1;
    case net_http_state_connecting:
    case net_http_state_established:
        return 0;
    }
}

uint8_t net_trans_http_endpoint_is_active(net_trans_http_endpoint_t trans_http) {
    switch(net_http_endpoint_state(net_http_endpoint_from_data(trans_http))) {
    case net_http_state_disable:
    case net_http_state_error:
        return 0;
    default:
        return 1;
    }
}
