#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_ssl_protocol_i.h"
#include "net_ssl_endpoint_i.h"
#include "net_ssl_endpoint_bio.h"

int net_ssl_protocol_init(net_protocol_t base_protocol) {
    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = NULL;
    protocol->m_em = NULL;
    protocol->m_svr.m_pkey = NULL;
    protocol->m_svr.m_cert_loaded = 0;

    protocol->m_bio_method = BIO_meth_new(58, net_protocol_name(base_protocol));
    if (protocol->m_bio_method == NULL) {
        CPE_ERROR(
            net_schedule_em(net_protocol_schedule(base_protocol)),
            "ssl: protocol: init: create bio method fail");
        return -1;
    }
    BIO_meth_set_write(protocol->m_bio_method, net_ssl_endpoint_bio_write);
    BIO_meth_set_read(protocol->m_bio_method, net_ssl_endpoint_bio_read);
    BIO_meth_set_puts(protocol->m_bio_method, net_ssl_endpoint_bio_puts);
    BIO_meth_set_ctrl(protocol->m_bio_method, net_ssl_endpoint_bio_ctrl);
    BIO_meth_set_create(protocol->m_bio_method, net_ssl_endpoint_bio_new);
    BIO_meth_set_destroy(protocol->m_bio_method, net_ssl_endpoint_bio_free);
    
    protocol->m_ssl_ctx = SSL_CTX_new(SSLv23_method());
    if(protocol->m_ssl_ctx == NULL) {
        CPE_ERROR(
            net_schedule_em(net_protocol_schedule(base_protocol)),
            "ssl: protocol: init: create ssl ctx fail");
        BIO_meth_free(protocol->m_bio_method);
        return -1;
    }

    SSL_CTX_set_options(
        protocol->m_ssl_ctx,
        SSL_OP_ALL
        | SSL_OP_NO_SSLv2
        | SSL_OP_NO_SSLv3
        | SSL_OP_NO_COMPRESSION
        | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

    //SSL_CTX_set_next_proto_select_cb(protocol->m_ssl_ctx, sfox_sfox_protocol_select_next_proto_cb, protocol);

    SSL_CTX_set_alpn_protos(protocol->m_ssl_ctx, (const unsigned char *)"\x02h2", 3);
    
    return 0;
}

void net_ssl_protocol_fini(net_protocol_t base_protocol) {
    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);

    if (protocol->m_bio_method) {
        BIO_meth_free(protocol->m_bio_method);
        protocol->m_bio_method = NULL;
    }

    if (protocol->m_ssl_ctx) {
        SSL_CTX_free(protocol->m_ssl_ctx);
        protocol->m_ssl_ctx = NULL;
    }

    if (protocol->m_svr.m_pkey) {
        EVP_PKEY_free(protocol->m_svr.m_pkey);
        protocol->m_svr.m_pkey = NULL;
    }
}

net_ssl_protocol_t
net_ssl_protocol_create(
    net_schedule_t schedule, const char * addition_name,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl");
    }

    net_protocol_t base_protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_ssl_protocol),
            net_ssl_protocol_init,
            net_ssl_protocol_fini,
            /*endpoint*/
            sizeof(struct net_ssl_endpoint),
            net_ssl_endpoint_init,
            net_ssl_endpoint_fini,
            net_ssl_endpoint_input,
            net_ssl_endpoint_on_state_change,
            NULL);

    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = alloc;
    protocol->m_em = em;
    return protocol;
}

void net_ssl_protocol_free(net_ssl_protocol_t protocol) {
    net_protocol_free(net_protocol_from_data(protocol));
}

net_ssl_protocol_t
net_ssl_protocol_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl");
    }

    net_protocol_t protocol = net_protocol_find(schedule, name);
    return protocol ? net_ssl_protocol_cast(protocol) : NULL;
}

net_ssl_protocol_t
net_ssl_protocol_cast(net_protocol_t base_protocol) {
    return net_protocol_init_fun(base_protocol) == net_ssl_protocol_init
        ? net_protocol_data(base_protocol)
        : NULL;
}

int net_ssl_protocol_svr_use_pkey_from_string(net_ssl_protocol_t protocol, const char * key) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);

    if (protocol->m_svr.m_pkey != NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use pkey: already setted!",
            net_protocol_name(base_protocol));
        return -1;
    }

    protocol->m_svr.m_pkey = net_ssl_pkey_from_string(protocol->m_em, key);
    if (protocol->m_svr.m_pkey == NULL) return -1;

    if (SSL_CTX_use_PrivateKey(protocol->m_ssl_ctx, protocol->m_svr.m_pkey) <= 0) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use pkey failed, %s",
            net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    return 0;
}

int net_ssl_protocol_svr_confirm_pkey(net_ssl_protocol_t protocol) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);
    
    if (protocol->m_svr.m_pkey != NULL) return 0;

    protocol->m_svr.m_pkey = net_ssl_pkey_generate(protocol->m_em);
    if (protocol->m_svr.m_pkey == NULL) return -1;

    if (SSL_CTX_use_PrivateKey(protocol->m_ssl_ctx, protocol->m_svr.m_pkey) <= 0) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use pkey failed, %s",
            net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    return 0;
}

int net_ssl_protocol_use_cert_from_string(net_ssl_protocol_t protocol, const char * cert) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);
    net_schedule_t schedule = net_protocol_schedule(base_protocol);

    if (protocol->m_svr.m_cert_loaded) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use cert: already setted!",
            net_protocol_name(base_protocol));
        return -1;
    }

    pem_password_cb * passwd_callback = SSL_CTX_get_default_passwd_cb(protocol->m_ssl_ctx);
    void * passwd_callback_userdata = SSL_CTX_get_default_passwd_cb_userdata(protocol->m_ssl_ctx);
    
    ERR_clear_error(); /* clear error stack for SSL_CTX_use_certificate() */
    BIO * cert_bio = BIO_new_mem_buf(cert, (int)strlen(cert));
    if (cert_bio == NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: init cert bio failed: %s",
            net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }
    
    X509 * x = PEM_read_bio_X509_AUX(cert_bio, NULL, passwd_callback, passwd_callback_userdata);
    if (x == NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: read cert failed: %s\n%s",
            net_protocol_name(base_protocol),
            ERR_error_string(ERR_get_error(), NULL), cert);
        BIO_free(cert_bio);
        return -1;
    }

    CPE_INFO(
        protocol->m_em, "ssl: %s: load cert: %s",
        net_protocol_name(base_protocol),
        net_ssl_dump_cert_info(net_schedule_tmp_buffer(schedule), x));
        
    if (SSL_CTX_use_certificate(protocol->m_ssl_ctx, x) != 1 || ERR_peek_error() != 0) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: read cert failed: %s",
            net_protocol_name(base_protocol),
            ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }

    /*
     * If we could set up our certificate, now proceed to the CA
     * certificates.
     */
    if (SSL_CTX_clear_chain_certs(protocol->m_ssl_ctx) != 1) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: clear chain certs fail: %s",
            net_protocol_name(base_protocol),
            ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }

    X509 *ca;
    while ((ca = PEM_read_bio_X509(cert_bio, NULL, passwd_callback, passwd_callback_userdata)) != NULL) {
        if (!SSL_CTX_add0_chain_cert(protocol->m_ssl_ctx, ca)) {
            CPE_ERROR(
                protocol->m_em, "ssl: %s: add ca cert fail: %s",
                net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL));
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
            protocol->m_em, "ssl: %s: last found error: %s",
            net_protocol_name(base_protocol), ERR_error_string(err, NULL));
        goto PROCESS_ERROR;
    }

    BIO_free(cert_bio);
    X509_free(x);

    protocol->m_svr.m_cert_loaded = 1;
    return 0;

PROCESS_ERROR:
    BIO_free(cert_bio);
    X509_free(x);
    return -1;
}

int net_ssl_protocol_svr_confirm_cert(net_ssl_protocol_t protocol) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);
    net_schedule_t schedule = net_protocol_schedule(base_protocol);
    
    if (protocol->m_svr.m_cert_loaded) return 0;

    X509 * x509 = NULL;

    ERR_clear_error(); /* clear error stack for SSL_CTX_use_certificate() */
    x509 = X509_new();
    if (x509 == NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: generate cert: X509_new error: %s",
            net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    X509_gmtime_adj(X509_getm_notBefore(x509), 0);
    X509_gmtime_adj(X509_getm_notAfter(x509), 31536000L);

    X509_set_pubkey(x509, protocol->m_svr.m_pkey);

    X509_NAME * name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"CA", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"sfox.org", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"localhost", -1, -1, 0);

    /* Now set the issuer name. */
    X509_set_issuer_name(x509, name);

    /* Actually sign the certificate with our key. */
    if (!X509_sign(x509, protocol->m_svr.m_pkey, EVP_sha1())) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: generate cert: sign error: %s",
            net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }

    if (SSL_CTX_use_certificate(protocol->m_ssl_ctx, x509) != 1 || ERR_peek_error() != 0) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: generate cert: SSL_CTX_use_certificate failed: %s",
            net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL));
        goto PROCESS_ERROR;
    }
    
    CPE_INFO(
        protocol->m_em, "ssl: %s: generate cert: %s",
        net_protocol_name(base_protocol),
        net_ssl_dump_cert_info(net_schedule_tmp_buffer(schedule), x509));
        
    X509_free(x509);
    return 0;

PROCESS_ERROR:
    if (x509) {
        X509_free(x509);
    }
    
    return -1;
}

int net_ssl_protocol_svr_prepaired(net_ssl_protocol_t protocol) {
    if (net_ssl_protocol_svr_confirm_pkey(protocol) != 0) return -1;
    if (net_ssl_protocol_svr_confirm_cert(protocol) != 0) return -1;
    return 0;
}

mem_buffer_t net_ssl_protocol_tmp_buffer(net_ssl_protocol_t protocol) {
    return net_schedule_tmp_buffer(net_protocol_schedule(net_protocol_from_data(protocol)));
}
