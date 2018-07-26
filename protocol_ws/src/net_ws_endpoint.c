#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_ws_endpoint_i.h"

net_ws_endpoint_t
net_ws_endpoint_create(net_driver_t driver, net_endpoint_type_t type, net_ws_protocol_t ws_protocol, const char * url) {
    net_endpoint_t endpoint = net_endpoint_create(driver, type, net_protocol_from_data(ws_protocol));
    if (endpoint == NULL) return NULL;
    
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);
    net_ws_endpoint_t ws_ep = net_endpoint_data(endpoint);

    ws_ep->m_url = cpe_str_mem_dup(alloc, url);
    if (ws_ep->m_url == NULL) {
        CPE_ERROR(net_schedule_em(schedule), "net_ws_endpoint_create: dup url %s fail", url);
        net_endpoint_free(endpoint);
        return NULL;
    }

    printf("xxxx:ws_ep = %p\n", ws_ep);
    return ws_ep;
}

void net_ws_endpoint_free(net_ws_endpoint_t ws_ep) {
    return net_endpoint_free(net_endpoint_from_data(ws_ep));
}

net_ws_endpoint_t net_ws_endpoint_get(net_endpoint_t endpoint) {
    return net_endpoint_data(endpoint);
}

void * net_ws_endpoint_data(net_ws_endpoint_t ws_ep) {
    return ws_ep + 1;
}

net_ws_endpoint_t net_ws_endpoint_from_data(void * data) {
    return ((net_ws_endpoint_t)data) - 1;
}

const char * net_ws_endpoint_url(net_ws_endpoint_t ws_ep) {
    return ws_ep->m_url;
}

int net_ws_endpoint_init(net_endpoint_t endpoint) {
    net_ws_endpoint_t ws_ep = net_endpoint_data(endpoint);
    ws_ep->m_url = NULL;
    return 0;
}

void net_ws_endpoint_fini(net_endpoint_t endpoint) {
    net_ws_endpoint_t ws_ep = net_endpoint_data(endpoint);
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);
    
    if (ws_ep->m_url) {
        mem_free(alloc, ws_ep->m_url);
        ws_ep->m_url = NULL;
    }
}

int net_ws_endpoint_input(net_endpoint_t endpoint) {
    return 0;
}


