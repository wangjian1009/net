#ifndef NET_XKCP_CONFIG_H_INCLEDED
#define NET_XKCP_CONFIG_H_INCLEDED
#include "net_xkcp_types.h"

NET_BEGIN_DECL

enum net_xkcp_mode {
    net_xkcp_mode_normal,
    net_xkcp_mode_fast,
    net_xkcp_mode_fast2,
    net_xkcp_mode_fast3,
};

struct net_xkcp_config {
    net_xkcp_mode_t m_mode;
	uint16_t m_mtu;
    uint32_t m_send_wnd;
    uint32_t m_recv_wnd;
};

void net_xkcp_config_init_default(net_xkcp_config_t config);
uint8_t net_xkcp_config_validate(net_xkcp_config_t config, error_monitor_t em);

NET_END_DECL

#endif
