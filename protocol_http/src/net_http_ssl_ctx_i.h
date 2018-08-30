#ifndef NET_HTTP_SSL_I_H_INCLEDED
#define NET_HTTP_SSL_I_H_INCLEDED
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "net_http_ssl_ctx.h"
#include "net_http_endpoint_i.h"

NET_BEGIN_DECL

struct net_http_ssl_ctx {
    net_http_endpoint_t m_http_ep;
    TAILQ_ENTRY(net_http_ssl_ctx) m_next;
    mbedtls_x509_crt m_cacert;
    mbedtls_entropy_context m_entropy;
    mbedtls_ctr_drbg_context m_ctr_drbg;
    mbedtls_ssl_config m_conf;
    mbedtls_ssl_context m_ssl;
};

net_http_ssl_ctx_t net_http_ssl_ctx_create(net_http_endpoint_t http_ep);
void net_http_ssl_ctx_free(net_http_ssl_ctx_t ssl_ctx);
void net_http_ssl_ctx_real_free(net_http_ssl_ctx_t ssl_ctx);

NET_END_DECL

#endif
