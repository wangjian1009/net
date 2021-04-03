#include "mbedtls/debug.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_ssl_protocol_i.h"
#include "net_ssl_endpoint_i.h"
#include "net_ssl_utils.h"

int net_ssl_protocol_init(net_protocol_t base_protocol) {
    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = NULL;
    protocol->m_em = NULL;
    protocol->m_entropy = NULL;
    protocol->m_ctr_drbg = NULL;
    protocol->m_svr.m_pkey = NULL;
    protocol->m_svr.m_cert = NULL;
    return 0;
}

void net_ssl_protocol_fini(net_protocol_t base_protocol) {
    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);

    if (protocol->m_svr.m_cert) {
        mbedtls_x509_crt_free(protocol->m_svr.m_cert);
        mem_free(protocol->m_alloc, protocol->m_svr.m_cert);
        protocol->m_svr.m_cert = NULL;
    }
    
    if (protocol->m_svr.m_pkey) {
        mbedtls_pk_free(protocol->m_svr.m_pkey);
        mem_free(protocol->m_alloc, protocol->m_svr.m_pkey);
        protocol->m_svr.m_pkey = NULL;
    }

    if (protocol->m_ctr_drbg) {
        mbedtls_ctr_drbg_free(protocol->m_ctr_drbg);
        mem_free(protocol->m_alloc, protocol->m_ctr_drbg);
        protocol->m_ctr_drbg = NULL;
    }

    if (protocol->m_entropy) {
        mbedtls_entropy_free(protocol->m_entropy);
        mem_free(protocol->m_alloc, protocol->m_entropy);
        protocol->m_entropy = NULL;
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

    int rv;

    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = alloc;
    protocol->m_em = em;

    /*entropy*/
    protocol->m_entropy = mem_alloc(protocol->m_alloc, sizeof(mbedtls_entropy_context));
    if (protocol->m_entropy == NULL) {
        CPE_ERROR(em, "ssl: %s: protocol: alloc mbedtls_entropy_context fail", net_protocol_name(base_protocol));
        goto INIT_ERROR;
    }
    mbedtls_entropy_init(protocol->m_entropy);

    mbedtls_debug_set_threshold(2);
    
    /*ctr_drgb*/
    protocol->m_ctr_drbg = mem_alloc(alloc, sizeof(mbedtls_ctr_drbg_context));
    if (protocol->m_ctr_drbg == NULL) {
        CPE_ERROR(em, "ssl: %s: protocol: alloc mbedtls_ctr_drbg_context fail", net_protocol_name(base_protocol));
        goto INIT_ERROR;
    }
    mbedtls_ctr_drbg_init(protocol->m_ctr_drbg);

    const char * DRBG_PERSONALIZED_STR = "Mbed TLS SSL_PROTOCOL";
    rv = mbedtls_ctr_drbg_seed(
        protocol->m_ctr_drbg, mbedtls_entropy_func, protocol->m_entropy,
        (const unsigned char *)DRBG_PERSONALIZED_STR, strlen(DRBG_PERSONALIZED_STR) + 1);
    if (rv != 0) {
        char error_buf[1024];
        CPE_ERROR(em, "ssl: %s: protocol: mbedtls_ctr_drbg_seed returned -0x%04X(%s)",
                  net_protocol_name(base_protocol), -rv, net_ssl_strerror(error_buf, sizeof(error_buf), rv));
        goto INIT_ERROR;
    }

    //SSL_CTX_set_next_proto_select_cb(protocol->m_ssl_ctx, sfox_sfox_protocol_select_next_proto_cb, protocol);
    /* SSL_CTX_set_alpn_protos(protocol->m_ssl_ctx, (const unsigned char *)"\x02h2", 3); */
    
    return protocol;

INIT_ERROR:
    net_protocol_free(base_protocol);
    return NULL;
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

    protocol->m_svr.m_pkey = mem_alloc(protocol->m_alloc, sizeof(mbedtls_pk_context));
    if (protocol->m_svr.m_pkey == NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use pkey: alloc fail!",
            net_protocol_name(base_protocol));
        return -1;
    }
    mbedtls_pk_init(protocol->m_svr.m_pkey);

    int rv;
    if ((rv = mbedtls_pk_parse_key(
             protocol->m_svr.m_pkey,
             (const unsigned char *)key, strlen(key),
             NULL, 0)) < 0)
    {
        char error_buf[1024];
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use pkey: mbedtls_pk_parse_key() returned -0x%04X(%s), key=%s!",
            net_protocol_name(base_protocol), -rv, net_ssl_strerror(error_buf, sizeof(error_buf), rv), key);
        goto INIT_FAILED;
    }

    return 0;

INIT_FAILED:
    mbedtls_pk_free(protocol->m_svr.m_pkey);
    mem_free(protocol->m_alloc, protocol->m_svr.m_pkey);
    protocol->m_svr.m_pkey = NULL;
    return -1;
}

int net_ssl_protocol_svr_confirm_pkey(net_ssl_protocol_t protocol) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);
    
    if (protocol->m_svr.m_pkey != NULL) return 0;

    protocol->m_svr.m_pkey = net_ssl_pkey_generate_rsa(protocol);
    if (protocol->m_svr.m_pkey == NULL) return -1;

    if (net_protocol_debug(base_protocol)) {
        /* CPE_INFO( */
        /*     protocol->m_em, "ssl: %s: confirm pkey: generated %s", */
        /*     net_protocol_name(base_protocol),  */
    }
    
    /* if (SSL_CTX_use_PrivateKey(protocol->m_ssl_ctx, protocol->m_svr.m_pkey) <= 0) { */
    /*     CPE_ERROR( */
    /*         protocol->m_em, "ssl: %s: use pkey failed, %s", */
    /*         net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL)); */
    /*     return -1; */
    /* } */

    return 0;
}

int net_ssl_protocol_use_cert_from_string(net_ssl_protocol_t protocol, const char * cert) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);
    net_schedule_t schedule = net_protocol_schedule(base_protocol);

    if (protocol->m_svr.m_cert != NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use cert: already setted!",
            net_protocol_name(base_protocol));
        return -1;
    }

    protocol->m_svr.m_cert = mem_alloc(protocol->m_alloc, sizeof(mbedtls_x509_crt));
    if (protocol->m_svr.m_cert == NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use cert: alloc fail!",
            net_protocol_name(base_protocol));
        return -1;
    }
    mbedtls_x509_crt_init(protocol->m_svr.m_cert);

    int rv = mbedtls_x509_crt_parse(protocol->m_svr.m_cert, (const unsigned char *)cert, strlen(cert) + 1);
    if (rv != 0) {
        char error_buf[1024];
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use pkey: mbedtls_x509_crt_parse() returned -0x%04X(%s)",
            net_protocol_name(base_protocol), -rv, net_ssl_strerror(error_buf, sizeof(error_buf), rv));
        goto PROCESS_ERROR;
    }
    
    /* CPE_INFO( */
    /*     protocol->m_em, "ssl: %s: load cert: %s", */
    /*     net_protocol_name(base_protocol), */
    /*     net_ssl_dump_cert_info(net_ssl_protocol_tmp_buffer(protocol), protocol->m_svr.m_cert)); */

/*     /\* */
/*      * If we could set up our certificate, now proceed to the CA */
/*      * certificates. */
/*      *\/ */
/*     if (SSL_CTX_clear_chain_certs(protocol->m_ssl_ctx) != 1) { */
/*         CPE_ERROR( */
/*             protocol->m_em, "ssl: %s: clear chain certs fail: %s", */
/*             net_protocol_name(base_protocol), */
/*             ERR_error_string(ERR_get_error(), NULL)); */
/*         goto PROCESS_ERROR; */
/*     } */

/*     X509 *ca; */
/*     while ((ca = PEM_read_bio_X509(cert_bio, NULL, passwd_callback, passwd_callback_userdata)) != NULL) { */
/*         if (!SSL_CTX_add0_chain_cert(protocol->m_ssl_ctx, ca)) { */
/*             CPE_ERROR( */
/*                 protocol->m_em, "ssl: %s: add ca cert fail: %s", */
/*                 net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL)); */
/*             X509_free(ca); */
/*             goto PROCESS_ERROR; */
/*         } */
/*     } */

    return 0;

PROCESS_ERROR:
    mbedtls_x509_crt_free(protocol->m_svr.m_cert);
    mem_free(protocol->m_alloc, protocol->m_svr.m_cert);
    protocol->m_svr.m_cert = NULL;
    return -1;
}

int net_ssl_protocol_svr_confirm_cert(net_ssl_protocol_t protocol) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);
    net_schedule_t schedule = net_protocol_schedule(base_protocol);
    
    if (protocol->m_svr.m_cert != NULL) return 0;

    char cert_buf[4096];

    const char * cert =
        net_ssl_cert_generate_selfsign(
            protocol, cert_buf, sizeof(cert_buf),
            1, x509_crt_version_3,
            "CN=Cert,O=mbed TLS,C=UK", protocol->m_svr.m_pkey);
    if (cert == NULL) {
        return -1;
    }

    return net_ssl_protocol_use_cert_from_string(protocol, cert);
}

int net_ssl_protocol_svr_prepaired(net_ssl_protocol_t protocol) {
    if (net_ssl_protocol_svr_confirm_pkey(protocol) != 0) return -1;
    if (net_ssl_protocol_svr_confirm_cert(protocol) != 0) return -1;
    return 0;
}

mem_buffer_t net_ssl_protocol_tmp_buffer(net_ssl_protocol_t protocol) {
    return net_schedule_tmp_buffer(net_protocol_schedule(net_protocol_from_data(protocol)));
}
