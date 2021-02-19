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
    net_schedule_t schedule = net_protocol_schedule(base_protocol);
    net_smux_protocol_t protocol = net_protocol_data(base_protocol);

    protocol->m_alloc = net_schedule_allocrator(schedule);
    protocol->m_em = net_schedule_em(schedule);

    net_smux_config_init_default(&protocol->m_dft_config);
    
    protocol->m_max_session_id = 0;
    TAILQ_INIT(&protocol->m_sessions);
    TAILQ_INIT(&protocol->m_dgrams);
    TAILQ_INIT(&protocol->m_free_frames);

    mem_buffer_init(&protocol->m_data_buffer, protocol->m_alloc);

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

    mem_buffer_clear(&protocol->m_data_buffer);
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

const struct net_smux_config *
net_smux_protocol_dft_config(net_smux_protocol_t protocol) {
    return &protocol->m_dft_config;
}

int net_smux_protocol_set_dft_config(net_smux_protocol_t protocol, net_smux_config_t cfg) {
    assert(cfg);

    if (!net_smux_config_validate(cfg, protocol->m_em)) {
        CPE_ERROR(
            protocol->m_em, "smux: set dft config: config error!");
        return -1;
    }

    protocol->m_dft_config = *cfg;
    return 0;
}

mem_buffer_t net_smux_protocol_tmp_buffer(net_smux_protocol_t protocol) {
    return net_schedule_tmp_buffer(net_protocol_schedule(net_protocol_from_data(protocol)));
}

void net_smux_config_init_default(net_smux_config_t config) {
    config->m_version = 1;
    config->m_keep_alive_disabled = 0;
    config->m_keep_alive_interval_ms = 10 * 60 * 1000;
    config->m_keep_alive_timeout_ms = 30 * 60 * 1000;
    config->m_max_frame_size = 32768;
    config->m_max_recv_buffer = 4194304;
    config->m_max_stream_buffer = 65536;
}

uint8_t net_smux_config_validate(net_smux_config_t config, error_monitor_t em) {
	if (config->m_version != 1 && config->m_version != 2) {
        CPE_ERROR(em, "smux: validate config: version %d not support", config->m_version);
        return 0;
	}
    
	if (!config->m_keep_alive_disabled) {
		if (config->m_keep_alive_interval_ms == 0) {
			CPE_ERROR(em, "smux: validate config: keep-alive interval must be positive");
            return 0;
		}

		if (config->m_keep_alive_timeout_ms < config->m_keep_alive_interval_ms) {
			CPE_ERROR(em, "smux: validate config: keep-alive timeout must be larger than keep-alive interval");
            return 0;
		}
	}

	if (config->m_max_frame_size <= 0) {
		CPE_ERROR(em, "smux: validate config: max frame size must be positive");
        return 0;
	}
	if (config->m_max_frame_size > 65535) {
		CPE_ERROR(em, "smux: validate config: max frame size must not be larger than 65535");
        return 0;
	}
    
	if (config->m_max_recv_buffer <= 0) {
        CPE_ERROR(em, "smux: validate config: max receive buffer must be positive");
        return 0;
	}

	if (config->m_max_stream_buffer <= 0) {
		CPE_ERROR(em, "smux: validate config: max stream buffer must be positive");
        return 0;
	}
	if (config->m_max_stream_buffer > config->m_max_recv_buffer) {
		CPE_ERROR(em, "smux: validate config: max stream buffer must not be larger than max receive buffer");
        return 0;
	}
	if (config->m_max_stream_buffer > INT32_MAX) {
        CPE_ERROR(em, "smux: validate config: max stream buffer cannot be larger than 2147483647");
        return 0;
	}

    return 1;
}

const char * net_smux_runing_mode_str(net_smux_runing_mode_t runing_mode) {
    switch (runing_mode) {
    case net_smux_runing_mode_cli:
        return "cli";
    case net_smux_runing_mode_svr:
        return "svr";
    }
}
