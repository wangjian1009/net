#ifndef NET_SSL_PROTOCOL_I_H_INCLEDED
#define NET_SSL_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "net_ssl_protocol.h"
#include "net_ssl_utils.h"

struct net_ssl_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    BIO_METHOD * m_bio_method;
    SSL_CTX * m_ssl_ctx;
    struct {
        EVP_PKEY * m_pkey;
        uint8_t m_cert_loaded;
    } m_svr;
};

mem_buffer_t net_ssl_protocol_tmp_buffer(net_ssl_protocol_t protocol);

#endif
