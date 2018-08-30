#include "net_schedule.h"
#include "net_protocol.h"
#include "net_http_endpoint_i.h"
#include "net_http_req_i.h"
#include "net_http_ssl_ctx_i.h"

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
    net_http_endpoint_input_fun_t endpoint_upgraded_input,
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

    net_http_protocol_t http_protocol = net_protocol_data(protocol);
    http_protocol->m_protocol_capacity = protocol_capacity;
    http_protocol->m_protocol_init = protocol_init;
    http_protocol->m_protocol_fini = protocol_fini;
    http_protocol->m_endpoint_capacity = endpoint_capacity;
    http_protocol->m_endpoint_init = endpoint_init;
    http_protocol->m_endpoint_fini = endpoint_fini;
    http_protocol->m_endpoint_upgraded_input = endpoint_upgraded_input;
    http_protocol->m_endpoint_on_state_change = endpoint_on_state_change;

    if (http_protocol->m_protocol_init) {
        if (http_protocol->m_protocol_init(http_protocol) != 0) {
            CPE_ERROR(http_protocol->m_em, "http: protocol: %s: external init fail!", protocol_name);
            http_protocol->m_protocol_fini = NULL;
            net_protocol_free(protocol);
            return NULL;
        }
    }
    
    return http_protocol;
}

void net_http_protocol_free(net_http_protocol_t http_protocol) {
    net_protocol_free(net_protocol_from_data(http_protocol));
}

static int net_http_protocol_init(net_protocol_t protocol) {
    net_schedule_t schedule = net_protocol_schedule(protocol);
    net_http_protocol_t http_protocol = net_protocol_data(protocol);

    http_protocol->m_alloc = net_schedule_allocrator(schedule);
    http_protocol->m_em = net_schedule_em(schedule);

    http_protocol->m_protocol_capacity = 0;
    http_protocol->m_protocol_init = NULL;
    http_protocol->m_protocol_fini = NULL;
    http_protocol->m_endpoint_capacity = 0;
    http_protocol->m_endpoint_init = NULL;
    http_protocol->m_endpoint_fini = NULL;
    http_protocol->m_endpoint_on_state_change = NULL;

    TAILQ_INIT(&http_protocol->m_free_reqs);
    TAILQ_INIT(&http_protocol->m_free_ssl_ctxes);
    
    return 0;
}

static void net_http_protocol_fini(net_protocol_t protocol) {
    net_http_protocol_t http_protocol = net_protocol_data(protocol);

    if (http_protocol->m_protocol_fini) {
        http_protocol->m_protocol_fini(http_protocol);
    }
    
    while(!TAILQ_EMPTY(&http_protocol->m_free_reqs)) {
        net_http_req_real_free(TAILQ_FIRST(&http_protocol->m_free_reqs));
    }
    
    while(!TAILQ_EMPTY(&http_protocol->m_free_ssl_ctxes)) {
        net_http_ssl_ctx_real_free(TAILQ_FIRST(&http_protocol->m_free_ssl_ctxes));
    }
}

void * net_http_protocol_data(net_http_protocol_t http_protocol) {
    return http_protocol + 1;
}

net_http_protocol_t net_http_protocol_from_data(void * data) {
    return ((net_http_protocol_t)data) - 1;
}

mem_buffer_t net_http_protocol_tmp_buffer(net_http_protocol_t http_protocol) {
    return net_schedule_tmp_buffer(net_protocol_schedule(net_protocol_from_data(http_protocol)));
}

net_schedule_t net_http_protocol_schedule(net_http_protocol_t http_protocol) {
    return net_protocol_schedule(net_protocol_from_data(http_protocol));
}
