#include "net_schedule.h"
#include "net_protocol.h"
#include "net_http_protocol_i.h"
#include "net_http_endpoint_i.h"

static int net_http_protocol_init(net_protocol_t protocol);
static void net_http_protocol_fini(net_protocol_t protocol);

net_http_protocol_t net_http_protocol_create(
    net_schedule_t schedule,
    const char * protocol_name,
    /*protocol*/
    uint16_t protocol_capacity,
    net_http_protocol_init_fun_t protocol_init,
    net_http_protocol_fini_fun_t protocol_fini,
    /*endpoint*/
    uint16_t endpoint_capacity,
    net_http_endpoint_init_fun_t endpoint_init,
    net_http_endpoint_fini_fun_t endpoint_fini,
    net_http_endpoint_on_state_change_fun_t endpoint_on_state_change)
{
    net_protocol_t protocol =
        net_protocol_create(
            schedule,
            protocol_name,
            /*protocol*/
            sizeof(struct net_http_protocol) + protocol_capacity,
            net_http_protocol_init,
            net_http_protocol_fini,
            /*endpoint*/
            sizeof(struct net_http_endpoint) + endpoint_capacity,
            net_http_endpoint_init,
            net_http_endpoint_fini,
            net_http_endpoint_input,
            NULL,
            NULL,
            net_http_endpoint_on_state_change);
    if (protocol == NULL) {
        return NULL;
    }

    net_http_protocol_t ws_protocol = net_protocol_data(protocol);
    ws_protocol->m_protocol_capacity = protocol_capacity;
    ws_protocol->m_protocol_init = protocol_init;
    ws_protocol->m_protocol_fini = protocol_fini;
    ws_protocol->m_endpoint_capacity = endpoint_capacity;
    ws_protocol->m_endpoint_init = endpoint_init;
    ws_protocol->m_endpoint_fini = endpoint_fini;
    ws_protocol->m_endpoint_on_state_change = endpoint_on_state_change;

    return ws_protocol;
}

void net_http_protocol_free(net_http_protocol_t ws_protocol) {
    net_protocol_free(net_protocol_from_data(ws_protocol));
}

static int net_http_protocol_init(net_protocol_t protocol) {
    net_schedule_t schedule = net_protocol_schedule(protocol);
    net_http_protocol_t ws_protocol = net_protocol_data(protocol);

    ws_protocol->m_alloc = net_schedule_allocrator(schedule);
    ws_protocol->m_em = net_schedule_em(schedule);
    mem_buffer_init(&ws_protocol->m_data_buffer, ws_protocol->m_alloc);

    ws_protocol->m_protocol_capacity = 0;
    ws_protocol->m_protocol_init = NULL;
    ws_protocol->m_protocol_fini = NULL;
    ws_protocol->m_endpoint_capacity = 0;
    ws_protocol->m_endpoint_init = NULL;
    ws_protocol->m_endpoint_fini = NULL;
    ws_protocol->m_endpoint_on_state_change = NULL;
    
    return 0;
}

static void net_http_protocol_fini(net_protocol_t protocol) {
    net_http_protocol_t ws_protocol = net_protocol_data(protocol);
    mem_buffer_clear(&ws_protocol->m_data_buffer);
}

void * net_http_protocol_data(net_http_protocol_t http_protocol) {
    return http_protocol + 1;
}

net_http_protocol_t net_http_protocol_from_data(void * data) {
    return ((net_http_protocol_t)data) - 1;
}

mem_buffer_t net_http_protocol_tmp_buffer(net_http_protocol_t ws_protocol) {
    return net_schedule_tmp_buffer(net_protocol_schedule(net_protocol_from_data(ws_protocol)));
}

net_schedule_t net_http_protocol_schedule(net_http_protocol_t ws_protocol) {
    return net_protocol_schedule(net_protocol_from_data(ws_protocol));
}
