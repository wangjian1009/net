#ifndef NET_SSL_UTILS_I_H_INCLEDED
#define NET_SSL_UTILS_I_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ssl_protocol_i.h"

mbedtls_pk_context * net_ssl_pkey_from_string(net_ssl_protocol_t protocol, const char * key);
mbedtls_pk_context * net_ssl_pkey_generate(net_ssl_protocol_t protocol);

const char * net_ssl_dump_cert_info(mem_buffer_t buffer, mbedtls_x509_crt * cert);

void net_ssl_dump_tls_info(
    error_monitor_t em, mem_buffer_t buffer, const char * prefix, uint8_t with_data,
    int direction, int ssl_ver, int content_type, const void *buf, size_t len, mbedtls_ssl_context *ssl);

const char * net_ssl_bio_ctrl_cmd_str(int cmd);

#endif

