#include "cpe/pal/pal_stdio.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_http_endpoint_i.h"
#include "net_http_req_i.h"

static int net_http_protocol_init(net_protocol_t protocol);
static void net_http_protocol_fini(net_protocol_t protocol);

net_http_protocol_t
net_http_protocol_create(net_schedule_t schedule, const char * name_postfix) {
    char name[64];
    snprintf(name, sizeof(name), "http-%s", name_postfix);

    net_protocol_t protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_http_protocol),
            net_http_protocol_init,
            net_http_protocol_fini,
            /*endpoint*/
            sizeof(struct net_http_endpoint),
            net_http_endpoint_init,
            net_http_endpoint_fini,
            net_http_endpoint_input,
            net_http_endpoint_on_state_change,
            net_http_endpoint_calc_size,
            NULL);
    if (protocol == NULL) {
        return NULL;
    }

    net_http_protocol_t http_protocol = net_protocol_data(protocol);
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

    http_protocol->m_max_req_id = 0;
    
    TAILQ_INIT(&http_protocol->m_free_reqs);

    mem_buffer_init(&http_protocol->m_data_buffer, http_protocol->m_alloc);

    return 0;
}

static void net_http_protocol_fini(net_protocol_t protocol) {
    net_http_protocol_t http_protocol = net_protocol_data(protocol);

    while(!TAILQ_EMPTY(&http_protocol->m_free_reqs)) {
        net_http_req_real_free(TAILQ_FIRST(&http_protocol->m_free_reqs));
    }

    mem_buffer_clear(&http_protocol->m_data_buffer);
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

net_http_protocol_t net_http_protocol_find(net_schedule_t schedule, const char * name_postfix) {
    char name[64];
    snprintf(name, sizeof(name), "http-%s", name_postfix);
    
    net_protocol_t base_protocol = net_protocol_find(schedule, name);
    return base_protocol ? net_protocol_data(base_protocol) : NULL;
}
