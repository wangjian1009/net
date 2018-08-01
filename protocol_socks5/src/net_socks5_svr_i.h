#ifndef NET_SOCKS5_PRO_I_H_INCLEDED
#define NET_SOCKS5_PRO_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_socks5_svr.h"

NET_BEGIN_DECL

struct net_socks5_svr {
    uint8_t m_use_acl;
    struct mem_buffer m_tmp_buffer;
};

NET_END_DECL

#endif
