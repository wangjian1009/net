#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_ssl_svr_driver_i.h"
#include "net_ssl_svr_acceptor_i.h"
#include "net_ssl_svr_endpoint_i.h"
#include "net_ssl_svr_underline_i.h"
#include "net_ssl_svr_underline_bio.h"
#include "net_ssl_utils.h"

static int net_ssl_svr_driver_init(net_driver_t driver);
static void net_ssl_svr_driver_fini(net_driver_t driver);

static int net_ssl_svr_driver_init_cert(
    net_driver_t base_driver, net_ssl_svr_driver_t driver, const char * key, const char * cert);

net_ssl_svr_driver_t
net_ssl_svr_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-svr-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl-svr");
    }

    net_driver_t base_driver =
        net_driver_create(
            schedule,
            name,
            /*driver*/
            sizeof(struct net_ssl_svr_driver),
            net_ssl_svr_driver_init,
            net_ssl_svr_driver_fini,
            NULL,
            /*timer*/
            0, NULL, NULL, NULL, NULL, NULL,
            /*acceptor*/
            sizeof(struct net_ssl_svr_acceptor),
            net_ssl_svr_acceptor_init,
            net_ssl_svr_acceptor_fini,
            /*endpoint*/
            sizeof(struct net_ssl_svr_endpoint),
            net_ssl_svr_endpoint_init,
            net_ssl_svr_endpoint_fini,
            NULL,
            net_ssl_svr_endpoint_update,
            net_ssl_svr_endpoint_set_no_delay,
            net_ssl_svr_endpoint_get_mss,
            /*dgram*/
            0, NULL, NULL, NULL,
            /*watcher*/
            0, NULL, NULL, NULL);

    net_ssl_svr_driver_t ssl_driver = net_driver_data(base_driver);

    ssl_driver->m_alloc = alloc;
    ssl_driver->m_em = em;
    ssl_driver->m_underline_driver = underline_driver;
    ssl_driver->m_underline_protocol = net_ssl_svr_underline_protocol_create(schedule, name, ssl_driver);

    return ssl_driver;
}

void net_ssl_svr_driver_free(net_ssl_svr_driver_t svr_driver) {
    net_driver_free(net_driver_from_data(svr_driver));
}

net_ssl_svr_driver_t
net_ssl_svr_driver_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-svr-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl-svr");
    }

    net_driver_t driver = net_driver_find(schedule, name);
    return driver ? net_driver_data(driver) : NULL;
}

static int net_ssl_svr_driver_init(net_driver_t base_driver) {
    net_ssl_svr_driver_t driver = net_driver_data(base_driver);
    driver->m_alloc = NULL;
    driver->m_em = NULL;
    driver->m_underline_driver = NULL;
    driver->m_underline_protocol = NULL;
    driver->m_pkey = NULL;
    driver->m_cert_loaded = 0;

    driver->m_bio_method = BIO_meth_new(58, "net-ssl-svr");
    if (driver->m_bio_method == NULL) {
        CPE_ERROR(
            net_schedule_em(net_driver_schedule(base_driver)),
            "net: ssl: driver: init: create bio method fail");
        return -1;
    }
    BIO_meth_set_write(driver->m_bio_method, net_ssl_svr_underline_bio_write);
    BIO_meth_set_read(driver->m_bio_method, net_ssl_svr_underline_bio_read);
    BIO_meth_set_puts(driver->m_bio_method, net_ssl_svr_underline_bio_puts);
    BIO_meth_set_ctrl(driver->m_bio_method, net_ssl_svr_underline_bio_ctrl);
    BIO_meth_set_create(driver->m_bio_method, net_ssl_svr_underline_bio_new);
    BIO_meth_set_destroy(driver->m_bio_method, net_ssl_svr_underline_bio_free);
    
    driver->m_ssl_ctx = SSL_CTX_new(SSLv23_server_method());
    if(driver->m_ssl_ctx == NULL) {
        CPE_ERROR(
            net_schedule_em(net_driver_schedule(base_driver)),
            "net: ssl: driver: init: create ssl ctx fail");
        BIO_meth_free(driver->m_bio_method);
        return -1;
    }
 
    SSL_CTX_set_options(
        driver->m_ssl_ctx,
        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3
        | SSL_OP_NO_COMPRESSION
        | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
   
    return 0;
}

static void net_ssl_svr_driver_fini(net_driver_t base_driver) {
    net_ssl_svr_driver_t driver = net_driver_data(base_driver);

    if (driver->m_underline_protocol) {
        net_protocol_free(driver->m_underline_protocol);
        driver->m_underline_protocol = NULL;
    }
    
    if (driver->m_bio_method) {
        BIO_meth_free(driver->m_bio_method);
        driver->m_bio_method = NULL;
    }

    if (driver->m_ssl_ctx) {
        SSL_CTX_free(driver->m_ssl_ctx);
        driver->m_ssl_ctx = NULL;
    }

    if (driver->m_pkey) {
        EVP_PKEY_free(driver->m_pkey);
        driver->m_pkey = NULL;
    }
}

int net_ssl_svr_driver_use_pkey_from_string(net_ssl_svr_driver_t driver, const char * key) {
    net_driver_t base_driver = net_driver_from_data(driver);

    if (driver->m_pkey != NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: use pkey: already setted!",
            net_driver_name(base_driver));
        return -1;
    }

    driver->m_pkey = net_ssl_pkey_from_string(driver->m_em, key);
    if (driver->m_pkey == NULL) return -1;

    if (SSL_CTX_use_PrivateKey(driver->m_ssl_ctx, driver->m_pkey) <= 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: use pkey failed, %s",
            net_driver_name(base_driver), ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    return 0;
}

int net_ssl_svr_driver_confirm_pkey(net_ssl_svr_driver_t driver) {
    net_driver_t base_driver = net_driver_from_data(driver);
    
    if (driver->m_pkey != NULL) return 0;

    driver->m_pkey = net_ssl_pkey_generate(driver->m_em);
    if (driver->m_pkey == NULL) return -1;

    if (SSL_CTX_use_PrivateKey(driver->m_ssl_ctx, driver->m_pkey) <= 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: use pkey failed, %s",
            net_driver_name(base_driver), ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    return 0;
}

int net_ssl_svr_driver_use_cert_from_string(net_ssl_svr_driver_t driver, const char * cert) {
    net_driver_t base_driver = net_driver_from_data(driver);
    net_schedule_t schedule = net_driver_schedule(base_driver);

    if (driver->m_cert_loaded) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: use cert: already setted!",
            net_driver_name(base_driver));
        return -1;
    }

    pem_password_cb * passwd_callback = SSL_CTX_get_default_passwd_cb(driver->m_ssl_ctx);
    void * passwd_callback_userdata = SSL_CTX_get_default_passwd_cb_userdata(driver->m_ssl_ctx);
    
    ERR_clear_error(); /* clear error stack for SSL_CTX_use_certificate() */
    BIO * cert_bio = BIO_new_mem_buf(cert, (int)strlen(cert));
    if (cert_bio == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: init cert bio failed: %s",
            net_driver_name(base_driver), ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }
    
    X509 * x = PEM_read_bio_X509_AUX(cert_bio, NULL, passwd_callback, passwd_callback_userdata);
    if (x == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: read cert failed: %s\n%s",
            net_driver_name(base_driver),
            ERR_error_string(ERR_get_error(), NULL), cert);
        BIO_free(cert_bio);
        return -1;
    }

    CPE_INFO(
        driver->m_em, "net: ssl: %s: load cert: %s",
        net_driver_name(base_driver),
        net_ssl_dump_cert_info(net_schedule_tmp_buffer(schedule), x));
        
    if (SSL_CTX_use_certificate(driver->m_ssl_ctx, x) != 1 || ERR_peek_error() != 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: read cert failed: %s",
            net_driver_name(base_driver),
            ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }

    /*
     * If we could set up our certificate, now proceed to the CA
     * certificates.
     */
    if (SSL_CTX_clear_chain_certs(driver->m_ssl_ctx) != 1) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: clear chain certs fail: %s",
            net_driver_name(base_driver),
            ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }

    X509 *ca;
    while ((ca = PEM_read_bio_X509(cert_bio, NULL, passwd_callback, passwd_callback_userdata)) != NULL) {
        if (!SSL_CTX_add0_chain_cert(driver->m_ssl_ctx, ca)) {
            CPE_ERROR(
                driver->m_em, "net: ssl: %s: add ca cert fail: %s",
                net_driver_name(base_driver), ERR_error_string(ERR_get_error(), NULL));
            X509_free(ca);
            goto PROCESS_ERROR;
        }
    }

    /* When the while loop ends, it's usually just EOF. */
    unsigned long err = ERR_peek_last_error();
    if (ERR_GET_LIB(err) == ERR_LIB_PEM && ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
        ERR_clear_error();
    }
    else {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: last found error: %s",
            net_driver_name(base_driver), ERR_error_string(err, NULL));
        goto PROCESS_ERROR;
    }

    BIO_free(cert_bio);
    X509_free(x);

    driver->m_cert_loaded = 1;
    return 0;

PROCESS_ERROR:
    BIO_free(cert_bio);
    X509_free(x);
    return -1;
}

int net_ssl_svr_driver_confirm_cert(net_ssl_svr_driver_t driver) {
    net_driver_t base_driver = net_driver_from_data(driver);
    net_schedule_t schedule = net_driver_schedule(base_driver);
    
    if (driver->m_cert_loaded) return 0;

    X509 * x509 = NULL;

    ERR_clear_error(); /* clear error stack for SSL_CTX_use_certificate() */
    x509 = X509_new();
    if (x509 == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: generate cert: X509_new error: %s",
            net_driver_name(base_driver), ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    X509_gmtime_adj(X509_getm_notBefore(x509), 0);
    X509_gmtime_adj(X509_getm_notAfter(x509), 31536000L);

    X509_set_pubkey(x509, driver->m_pkey);

    X509_NAME * name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"CA", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"sfox.org", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"localhost", -1, -1, 0);

    /* Now set the issuer name. */
    X509_set_issuer_name(x509, name);

    /* Actually sign the certificate with our key. */
    if (!X509_sign(x509, driver->m_pkey, EVP_sha1())) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: generate cert: sign error: %s",
            net_driver_name(base_driver), ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }

    if (SSL_CTX_use_certificate(driver->m_ssl_ctx, x509) != 1 || ERR_peek_error() != 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: generate cert: SSL_CTX_use_certificate failed: %s",
            net_driver_name(base_driver), ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }
    
    CPE_INFO(
        driver->m_em, "net: ssl: %s: generate cert: %s",
        net_driver_name(base_driver),
        net_ssl_dump_cert_info(net_schedule_tmp_buffer(schedule), x509));
        
    X509_free(x509);
    return 0;

PROCESS_ERROR:
    if (x509) {
        X509_free(x509);
    }
    
    return -1;
}

int net_ssl_svr_driver_prepaired(net_ssl_svr_driver_t driver) {
    if (net_ssl_svr_driver_confirm_pkey(driver) != 0) return -1;
    if (net_ssl_svr_driver_confirm_cert(driver) != 0) return -1;
    return 0;
}

mem_buffer_t net_ssl_svr_driver_tmp_buffer(net_ssl_svr_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
