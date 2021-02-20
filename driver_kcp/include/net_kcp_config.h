#ifndef NET_KCP_CONFIG_H_INCLEDED
#define NET_KCP_CONFIG_H_INCLEDED
#include "net_kcp_types.h"

NET_BEGIN_DECL

struct net_kcp_config {
	/* Listen       string `json:"listen"` */
	/* Target       string `json:"target"` */
	/* Key          string `json:"key"` */
	/* Crypt        string `json:"crypt"` */
	/* Mode         string `json:"mode"` */
	/* MTU          int    `json:"mtu"` */
	/* SndWnd       int    `json:"sndwnd"` */
	/* RcvWnd       int    `json:"rcvwnd"` */
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

NET_END_DECL

#endif
