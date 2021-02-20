#ifndef NET_XKCP_CONFIG_H_INCLEDED
#define NET_XKCP_CONFIG_H_INCLEDED
#include "net_xkcp_types.h"

NET_BEGIN_DECL

enum net_xkcp_mode {
    net_xkcp_mode_normal,
    net_xkcp_mode_fast,
    net_xkcp_mode_fast1,
    net_xkcp_mode_fast2,
};

struct net_xkcp_config {
	/* Listen       string `json:"listen"` */
	/* Target       string `json:"target"` */
	/* Key          string `json:"key"` */
	/* Crypt        string `json:"crypt"` */
    net_xkcp_mode_t m_mode;
	uint16_t m_mtu;
    uint32_t m_send_wnd;
    uint32_t m_recv_wnd;
	/* DataShard    int    `json:"datashard"` */
	/* ParityShard  int    `json:"parityshard"` */
	/* DSCP         int    `json:"dscp"` */
	/* NoComp       bool   `json:"nocomp"` */
	/* AckNodelay   bool   `json:"acknodelay"` */
	/* NoDelay      int    `json:"nodelay"` */
	/* Interval     int    `json:"interval"` */
	/* Resend       int    `json:"resend"` */
	/* NoCongestion int    `json:"nc"` */
	/* SockBuf      int    `json:"sockbuf"` */
    uint8_t m_smux_ver;
    uint32_t m_smux_recv_buffer;
    uint32_t m_smux_stream_buffer;
	/* KeepAlive    int    `json:"keepalive"` */
	/* Log          string `json:"log"` */
	/* TCP          bool   `json:"tcp"` */
};

void net_xkcp_config_init_default(net_xkcp_config_t config);
uint8_t net_xkcp_config_validate(net_xkcp_config_t config, error_monitor_t em);

NET_END_DECL

#endif
