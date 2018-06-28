#ifndef NET_SOCKS5_PRO_I_H_INCLEDED
#define NET_SOCKS5_PRO_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_socks5_svr.h"

NET_BEGIN_DECL

typedef struct net_socks5_svr_endpoint * net_socks5_svr_endpoint_t;
    
struct net_socks5_svr {
    uint8_t m_use_acl;
    net_socks5_svr_connect_fun_t m_dft_connect;
    void * m_dft_connect_ctx;
    struct mem_buffer m_tmp_buffer;
};

NET_END_DECL

#endif
