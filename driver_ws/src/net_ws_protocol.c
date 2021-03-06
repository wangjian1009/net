#include "cpe/pal/pal_stdio.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_ws_protocol_i.h"
#include "net_ws_endpoint_i.h"

int net_ws_protocol_init(net_protocol_t base_protocol) {
    net_ws_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = NULL;
    protocol->m_em = NULL;
    mem_buffer_init(&protocol->m_data_buffer, NULL);
    return 0;
}

void net_ws_protocol_fini(net_protocol_t base_protocol) {
    net_ws_protocol_t protocol = net_protocol_data(base_protocol);
    mem_buffer_clear(&protocol->m_data_buffer);
}

net_ws_protocol_t
net_ws_protocol_create(
    net_schedule_t schedule, const char * addition_name,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ws-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ws");
    }

    net_protocol_t base_protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_ws_protocol),
            net_ws_protocol_init,
            net_ws_protocol_fini,
            /*endpoint*/
            sizeof(struct net_ws_endpoint),
            net_ws_endpoint_init,
            net_ws_endpoint_fini,
            net_ws_endpoint_input,
            net_ws_endpoint_on_state_change,
            net_ws_endpoint_calc_size,
            NULL);

    net_ws_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = alloc;
    protocol->m_em = em;
    return protocol;
}

void net_ws_protocol_free(net_ws_protocol_t protocol) {
    net_protocol_free(net_protocol_from_data(protocol));
}

net_ws_protocol_t
net_ws_protocol_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ws-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ws");
    }

    net_protocol_t protocol = net_protocol_find(schedule, name);
    return protocol ? net_ws_protocol_cast(protocol) : NULL;
}

net_ws_protocol_t
net_ws_protocol_cast(net_protocol_t base_protocol) {
    return net_protocol_init_fun(base_protocol) == net_ws_protocol_init
        ? net_protocol_data(base_protocol)
        : NULL;
}

mem_buffer_t net_ws_protocol_tmp_buffer(net_ws_protocol_t protocol) {
    return net_schedule_tmp_buffer(net_protocol_schedule(net_protocol_from_data(protocol)));
}
