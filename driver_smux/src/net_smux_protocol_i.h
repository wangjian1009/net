#ifndef NET_SMUX_PROTOCOL_I_H_INCLEDED
#define NET_SMUX_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_smux_protocol.h"
#include "net_smux_config.h"

typedef enum net_smux_cmd net_smux_cmd_t;
typedef struct net_smux_frame * net_smux_frame_t;
typedef TAILQ_HEAD(net_smux_session_list, net_smux_session) net_smux_session_list_t;
typedef TAILQ_HEAD(net_smux_dgram_list, net_smux_dgram) net_smux_dgram_list_t;
typedef TAILQ_HEAD(net_smux_frame_list, net_smux_frame) net_smux_frame_list_t;

struct net_smux_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    struct net_smux_config m_dft_config;

    /*runtime*/
    uint32_t m_max_session_id;
    net_smux_session_list_t m_sessions;

    net_smux_dgram_list_t m_dgrams;

    /*frame cache*/
    net_smux_frame_list_t m_free_frames;

    /*data cache*/
    struct mem_buffer m_data_buffer;
};

net_schedule_t net_smux_protocol_schedule(net_smux_protocol_t protocol);
mem_buffer_t net_smux_protocol_tmp_buffer(net_smux_protocol_t protocol);

#endif
