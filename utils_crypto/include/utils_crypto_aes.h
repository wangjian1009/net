#ifndef NET_UTILS_CRYPTO_AES_H_INCLEDED
#define NET_UTILS_CRYPTO_AES_H_INCLEDED
#include "utils_crypto_system.h"

CPE_BEGIN_DECL

ssize_t utils_crypto_aes_encode(
    write_stream_t os, read_stream_t is,
    void const * passwd, uint32_t passwd_sz, net_crypto_pending_t pending,
    error_monitor_t em);

ssize_t utils_crypto_aes_decode(
    write_stream_t os, read_stream_t is,
    void const * passwd, uint32_t passwd_sz, net_crypto_pending_t pending,
    error_monitor_t em);

CPE_END_DECL

#endif
