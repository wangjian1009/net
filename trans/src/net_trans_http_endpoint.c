#include "assert.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_strings.h"
#include "net_http_endpoint.h"
#include "net_http_protocol.h"
#include "net_trans_http_endpoint_i.h"
#include "net_trans_host_i.h"

net_trans_http_endpoint_t
net_trans_http_endpoint_create(net_trans_host_t host) {
    net_trans_manage_t mgr = host->m_mgr;
    
    net_http_endpoint_t http_endpoint =
        net_http_endpoint_create(
            mgr->m_driver, net_endpoint_outbound, net_http_protocol_from_data(mgr->m_http_protocol));
    if (http_endpoint == NULL) {
        CPE_ERROR(mgr->m_em, "trans: create http-endpoint fail!");
        return NULL;
    }

    net_trans_http_endpoint_t trans_http = net_http_endpoint_data(http_endpoint);

    trans_http->m_host = host;
    TAILQ_INSERT_TAIL(&host->m_endpoints, trans_http, m_next);
    
    return trans_http;
}

void net_trans_http_endpoint_free(net_trans_http_endpoint_t trans_http) {
    net_http_endpoint_free(net_http_endpoint_from_data(trans_http));
}

int net_trans_http_endpoint_init(net_http_endpoint_t http_endpoint) {
    net_trans_http_endpoint_t trans_http = net_http_endpoint_data(http_endpoint);
    trans_http->m_host = NULL;
    return 0;
}

void net_trans_http_endpoint_fini(net_http_endpoint_t http_endpoint) {
    net_trans_http_endpoint_t trans_http = net_http_endpoint_data(http_endpoint);

    if (trans_http->m_host) {
        TAILQ_REMOVE(&trans_http->m_host->m_endpoints, trans_http, m_next);
        trans_http->m_host = NULL;
    }
}

int net_trans_http_endpoint_on_state_change(net_http_endpoint_t http_endpoint, net_http_state_t from_state) {
    return 0;
}
