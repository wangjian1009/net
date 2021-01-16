#ifndef NET_SSL_UTILS_I_H_INCLEDED
#define NET_SSL_UTILS_I_H_INCLEDED
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

void net_ssl_dump_tls_info(
    net_schedule_t schedule, const char * prefix,
    int direction, int ssl_ver, int content_type, const void *buf, size_t len, SSL *ssl);

const char * net_ssl_errno_str(int e);

#endif

