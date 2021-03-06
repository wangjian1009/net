#ifndef NET_SSL_UTILS_I_H_INCLEDED
#define NET_SSL_UTILS_I_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ssl_protocol_i.h"

enum x509_crt_version {
    x509_crt_version_1,
    x509_crt_version_2,
    x509_crt_version_3,
};

mbedtls_pk_context * net_ssl_pkey_generate_rsa(net_ssl_protocol_t protocol);

const char * net_ssl_strerror(char * buf, uint32_t buf_capacity, int code);

const char * net_ssl_cert_generate_selfsign(
    net_ssl_protocol_t protocol, char * buf, uint32_t buf_len,
    enum x509_crt_version ver,
    int64_t serial,
    const char * issuer_name, mbedtls_pk_context * issuer_pk);

const char * net_ssl_dump_cert_info(mem_buffer_t buffer, mbedtls_x509_crt * cert);

void net_ssl_dump_tls_info(
    error_monitor_t em, mem_buffer_t buffer, const char * prefix, uint8_t with_data,
    int direction, int ssl_ver, int content_type, const void *buf, size_t len, mbedtls_ssl_context *ssl);

#endif

