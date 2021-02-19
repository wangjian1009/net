#ifndef NET_SMUX_CONFIG_H_INCLEDED
#define NET_SMUX_CONFIG_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_smux_types.h"

struct net_smux_config {
	/* SMUX Protocol version, support 1,2 */
	uint8_t m_version;

	/* Disabled keepalive */
	uint8_t m_keep_alive_disabled;

	/* KeepAliveInterval is how often to send a NOP command to the remote */
    uint32_t m_keep_alive_interval_ms;

	/* KeepAliveTimeout is how long the session will be closed if no data has arrived */
    uint32_t m_keep_alive_timeout_ms;

	/* MaxFrameSize is used to control the maximum frame size to sent to the remote */
    uint32_t m_max_frame_size;

	/* MaxReceiveBuffer is used to control the maximum number of data in the buffer pool */
	uint32_t m_max_recv_buffer;

	/* MaxStreamBuffer is used to control the maximum number of data per stream*/
	uint32_t m_max_stream_buffer;
};

void net_smux_config_init_default(net_smux_config_t config);
uint8_t net_smux_config_validate(net_smux_config_t config, error_monitor_t em);

#endif
