#ifndef NET_SSL_SVR_DRIVER_I_H_INCLEDED
#define NET_SSL_SVR_DRIVER_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "net_ssl_svr_driver.h"
#include "net_ssl_utils.h"

typedef struct net_ssl_svr_acceptor * net_ssl_svr_acceptor_t;
typedef struct net_ssl_svr_undline * net_ssl_svr_undline_t;
typedef enum net_ssl_svr_endpoint_ssl_state net_ssl_svr_endpoint_ssl_state_t;

struct net_ssl_svr_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    BIO_METHOD * m_bio_method;
    SSL_CTX * m_ssl_ctx;
    net_driver_t m_underline_driver;
    net_protocol_t m_underline_protocol;
    uint8_t m_pkey_loaded;
    uint8_t m_cert_loaded;
};

int net_ssl_svr_driver_confirm_pkey(net_ssl_svr_driver_t driver);
int net_ssl_svr_driver_confirm_cert(net_ssl_svr_driver_t driver);

#endif
