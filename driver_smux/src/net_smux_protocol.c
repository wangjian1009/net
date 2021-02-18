#include <assert.h>
#include "cpe/pal/pal_stdio.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_smux_protocol_i.h"
#include "net_smux_endpoint_i.h"
#include "net_smux_session_i.h"
#include "net_smux_dgram_i.h"
#include "net_smux_frame_i.h"

int net_smux_protocol_init(net_protocol_t base_protocol) {
    net_smux_protocol_t protocol = net_protocol_data(base_protocol);

    protocol->m_alloc = NULL;
    protocol->m_em = NULL;

    protocol->m_cfg_version = 1;
    protocol->m_cfg_keep_alive_disabled = 0;
    protocol->m_cfg_keep_alive_interval_ms = 10 * 60 * 1000;
    protocol->m_cfg_keep_alive_timeout_ms = 30 * 60 * 1000;
    protocol->m_cfg_max_frame_size = 32768;
    protocol->m_cfg_max_session_buffer = 4194304;
    protocol->m_cfg_max_stream_buffer = 65536;

    protocol->m_max_session_id = 0;
    TAILQ_INIT(&protocol->m_sessions);
    TAILQ_INIT(&protocol->m_dgrams);
    TAILQ_INIT(&protocol->m_free_frames);

    return 0;
}

void net_smux_protocol_fini(net_protocol_t base_protocol) {
    net_smux_protocol_t protocol = net_protocol_data(base_protocol);

    while(!TAILQ_EMPTY(&protocol->m_dgrams)) {
        net_smux_dgram_free(TAILQ_FIRST(&protocol->m_dgrams));
    }

    assert(TAILQ_EMPTY(&protocol->m_sessions));

    while(!TAILQ_EMPTY(&protocol->m_free_frames)) {
        net_smux_frame_real_free(protocol, TAILQ_FIRST(&protocol->m_free_frames));
    }
}

net_smux_protocol_t
net_smux_protocol_create(
    net_schedule_t schedule, const char * addition_name,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "smux-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "smux");
    }

    net_protocol_t base_protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_smux_protocol),
            net_smux_protocol_init,
            net_smux_protocol_fini,
            /*endpoint*/
            sizeof(struct net_smux_endpoint),
            net_smux_endpoint_init,
            net_smux_endpoint_fini,
            net_smux_endpoint_input,
            net_smux_endpoint_on_state_change,
            NULL);

    net_smux_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = alloc;
    protocol->m_em = em;
    return protocol;
}

void net_smux_protocol_free(net_smux_protocol_t protocol) {
    net_protocol_free(net_protocol_from_data(protocol));
}

net_smux_protocol_t
net_smux_protocol_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "smux-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "smux");
    }

    net_protocol_t protocol = net_protocol_find(schedule, name);
    return protocol ? net_smux_protocol_cast(protocol) : NULL;
}

net_smux_protocol_t
net_smux_protocol_cast(net_protocol_t base_protocol) {
    return net_protocol_init_fun(base_protocol) == net_smux_protocol_init
        ? net_protocol_data(base_protocol)
        : NULL;
}

net_schedule_t net_smux_protocol_schedule(net_smux_protocol_t protocol) {
    return net_protocol_schedule(net_protocol_from_data(protocol));
}

mem_buffer_t net_smux_protocol_tmp_buffer(net_smux_protocol_t protocol) {
    return net_schedule_tmp_buffer(net_protocol_schedule(net_protocol_from_data(protocol)));
}
