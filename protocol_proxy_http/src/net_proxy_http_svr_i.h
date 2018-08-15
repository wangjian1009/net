#ifndef NET_PROXY_HTTP_SVR_I_H_INCLEDED
#define NET_PROXY_HTTP_SVR_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_proxy_http_svr.h"

NET_BEGIN_DECL

struct net_proxy_http_svr {
    uint8_t m_use_acl;
    struct mem_buffer m_tmp_buffer;
};

NET_END_DECL

#endif
