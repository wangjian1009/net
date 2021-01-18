#ifndef NET_SSL_UTILS_I_H_INCLEDED
#define NET_SSL_UTILS_I_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

EVP_PKEY * net_ssl_pkey_from_string(error_monitor_t em, const char * key);
EVP_PKEY * net_ssl_pkey_generate(error_monitor_t em);

const char * net_ssl_dump_cert_info(mem_buffer_t buffer, X509 * cert);

void net_ssl_dump_tls_info(
    error_monitor_t em, mem_buffer_t buffer, const char * prefix, uint8_t with_data,
    int direction, int ssl_ver, int content_type, const void *buf, size_t len, SSL *ssl);

const char * net_ssl_bio_ctrl_cmd_str(int cmd);

#endif

