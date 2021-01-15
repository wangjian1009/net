#ifndef NET_SSL_SVR_DRIVER_I_H_INCLEDED
#define NET_SSL_SVR_DRIVER_I_H_INCLEDED
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include "net_ssl_svr_driver.h"

typedef struct net_ssl_svr_acceptor * net_ssl_svr_acceptor_t;

struct net_ssl_svr_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    BIO_METHOD * m_bio_method;
    SSL_CTX * m_ssl_ctx;
    net_driver_t m_underline_driver;
};

#endif
