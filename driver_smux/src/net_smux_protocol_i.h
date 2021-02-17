#ifndef NET_SMUX_PROTOCOL_I_H_INCLEDED
#define NET_SMUX_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_smux_protocol.h"

typedef enum net_smux_cmd net_smux_cmd_t;
typedef TAILQ_HEAD(net_smux_session_list, net_smux_session) net_smux_session_list_t;

struct net_smux_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;

	/* SMUX Protocol version, support 1,2 */
	uint8_t m_cfg_version;

	/* Disabled keepalive */
	uint8_t m_cfg_keep_alive_disabled;

	/* KeepAliveInterval is how often to send a NOP command to the remote */
    uint32_t m_cfg_keep_alive_interval_ms;

	/* KeepAliveTimeout is how long the session will be closed if no data has arrived */
    uint32_t m_cfg_keep_alive_timeout_ms;

	/* MaxFrameSize is used to control the maximum frame size to sent to the remote */
    uint32_t m_cfg_max_frame_size;

	/* MaxReceiveBuffer is used to control the maximum number of data in the buffer pool */
	uint32_t m_cfg_max_session_buffer;

	/* MaxStreamBuffer is used to control the maximum number of data per stream*/
	uint32_t m_cfg_max_stream_buffer;

    uint32_t m_max_session_id;
    net_smux_session_list_t m_sessions;
};

net_schedule_t net_smux_protocol_schedule(net_smux_protocol_t protocol);
mem_buffer_t net_smux_protocol_tmp_buffer(net_smux_protocol_t protocol);

#endif
