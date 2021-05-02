#ifndef NET_SSL_PROTOCOL_I_H_INCLEDED
#define NET_SSL_PROTOCOL_I_H_INCLEDED
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "net_ssl_protocol.h"

struct net_ssl_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    mbedtls_entropy_context * m_entropy;
    mbedtls_ctr_drbg_context * m_ctr_drbg;
    uint16_t m_ciphersuite_capacity;
    uint16_t m_ciphersuite_count;
    int * m_ciphersuites;
    struct {
        mbedtls_pk_context * m_pkey;
        mbedtls_x509_crt * m_cert;
    } m_svr;
};

mem_buffer_t net_ssl_protocol_tmp_buffer(net_ssl_protocol_t protocol);

#endif
