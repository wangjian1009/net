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
    protocol->m_cli.m_ssl_config = NULL;
    protocol->m_svr.m_ssl_config = NULL;
    protocol->m_svr.m_pkey = NULL;
    protocol->m_svr.m_cert = NULL;
    return 0;
}

void net_ssl_protocol_fini(net_protocol_t base_protocol) {
    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);

    if (protocol->m_cli.m_ssl_config) {
        mbedtls_ssl_config_free(protocol->m_cli.m_ssl_config);
        mem_free(protocol->m_alloc, protocol->m_cli.m_ssl_config);
        protocol->m_cli.m_ssl_config = NULL;
    }

    if (protocol->m_svr.m_ssl_config) {
        mbedtls_ssl_config_free(protocol->m_svr.m_ssl_config);
        mem_free(protocol->m_alloc, protocol->m_svr.m_ssl_config);
        protocol->m_svr.m_ssl_config = NULL;
    }

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
        CPE_ERROR(em, "ssl: %s: protocol: mbedtls_ctr_drbg_seed returned -0x%04X", net_protocol_name(base_protocol), -rv);
        goto INIT_ERROR;
    }

    /*client ssl_config*/
    protocol->m_cli.m_ssl_config = mem_alloc(alloc, sizeof(mbedtls_ssl_config));
    if (protocol->m_cli.m_ssl_config == NULL) {
        CPE_ERROR(em, "ssl: %s: protocol: alloc client ssl_config fail", net_protocol_name(base_protocol));
        goto INIT_ERROR;
    }
    mbedtls_ssl_config_init(protocol->m_cli.m_ssl_config);

    rv = mbedtls_ssl_config_defaults(
        protocol->m_cli.m_ssl_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (rv != 0) {
        CPE_ERROR(
            em, "ssl: %s: protocol: alloc client ssl_config_defaults fail, -0x%04X",
            net_protocol_name(base_protocol), -rv);
        goto INIT_ERROR;
    }
    mbedtls_ssl_conf_rng(protocol->m_cli.m_ssl_config, mbedtls_ctr_drbg_random, protocol->m_ctr_drbg);
    
    /*server ssl_config*/
    protocol->m_svr.m_ssl_config = mem_alloc(alloc, sizeof(mbedtls_ssl_config));
    if (protocol->m_svr.m_ssl_config == NULL) {
        CPE_ERROR(em, "ssl: %s: protocol: alloc svrent ssl_config fail", net_protocol_name(base_protocol));
        goto INIT_ERROR;
    }
    mbedtls_ssl_config_init(protocol->m_svr.m_ssl_config);

    rv = mbedtls_ssl_config_defaults(
        protocol->m_svr.m_ssl_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (rv != 0) {
        CPE_ERROR(
            em, "ssl: %s: protocol: alloc svrent ssl_config_defaults fail, -0x%04X",
            net_protocol_name(base_protocol), -rv);
        goto INIT_ERROR;
    }
    mbedtls_ssl_conf_rng(protocol->m_svr.m_ssl_config, mbedtls_ctr_drbg_random, protocol->m_ctr_drbg);
    
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

    /* protocol->m_svr.m_pkey = net_ssl_pkey_from_string(protocol->m_em, key); */
    /* if (protocol->m_svr.m_pkey == NULL) return -1; */

    /* if (SSL_CTX_use_PrivateKey(protocol->m_ssl_ctx, protocol->m_svr.m_pkey) <= 0) { */
    /*     CPE_ERROR( */
    /*         protocol->m_em, "ssl: %s: use pkey failed, %s", */
    /*         net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL)); */
    /*     return -1; */
    /* } */

    return 0;
}

int net_ssl_protocol_svr_confirm_pkey(net_ssl_protocol_t protocol) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);
    
    if (protocol->m_svr.m_pkey != NULL) return 0;

    protocol->m_svr.m_pkey = net_ssl_pkey_generate(protocol);
    if (protocol->m_svr.m_pkey == NULL) return -1;

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
    if (protocol->m_svr.m_cert) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use cert: alloc fail!",
            net_protocol_name(base_protocol));
        return -1;
    }
    mbedtls_x509_crt_init(protocol->m_svr.m_cert);

    int rv = mbedtls_x509_crt_parse(protocol->m_svr.m_cert, (const unsigned char *)cert, strlen(cert) + 1);
    if (rv != 0) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: use pkey: mbedtls_x509_crt_parse() returned -0x%04X\n",
            net_protocol_name(base_protocol), -rv);
        goto PROCESS_ERROR;
    }
    
    CPE_INFO(
        protocol->m_em, "ssl: %s: load cert: %s",
        net_protocol_name(base_protocol),
        net_ssl_dump_cert_info(net_ssl_protocol_tmp_buffer(protocol), protocol->m_svr.m_cert));

    mbedtls_ssl_conf_ca_chain(protocol->m_svr.m_ssl_config, protocol->m_svr.m_cert, NULL);

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

    protocol->m_svr.m_cert = mem_alloc(protocol->m_alloc, sizeof(mbedtls_x509_crt));
    if (protocol->m_svr.m_cert) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: confirm cert: alloc fail!",
            net_protocol_name(base_protocol));
        return -1;
    }
    mbedtls_x509_crt_init(protocol->m_svr.m_cert);
    
/*     ASN1_INTEGER_set(X509_get_serialNumber(x509), 1); */

/*     X509_gmtime_adj(X509_getm_notBefore(x509), 0); */
/*     X509_gmtime_adj(X509_getm_notAfter(x509), 31536000L); */

    //X509_set_pubkey(protocol->m_svr.m_cert, protocol->m_svr.m_pkey);

/*     X509_NAME * name = X509_get_subject_name(x509); */
/*     X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"CA", -1, -1, 0); */
/*     X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"sfox.org", -1, -1, 0); */
/*     X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"localhost", -1, -1, 0); */

/*     /\* Now set the issuer name. *\/ */
/*     X509_set_issuer_name(x509, name); */

/*     /\* Actually sign the certificate with our key. *\/ */
/*     if (!X509_sign(x509, protocol->m_svr.m_pkey, EVP_sha1())) { */
/*         CPE_ERROR( */
/*             protocol->m_em, "ssl: %s: generate cert: sign error: %s", */
/*             net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL)); */
/*         goto PROCESS_ERROR; */
/*     } */

/*     if (SSL_CTX_use_certificate(protocol->m_ssl_ctx, x509) != 1 || ERR_peek_error() != 0) { */
/*         CPE_ERROR( */
/*             protocol->m_em, "ssl: %s: generate cert: SSL_CTX_use_certificate failed: %s", */
/*             net_protocol_name(base_protocol), ERR_error_string(ERR_get_error(), NULL)); */
/*         goto PROCESS_ERROR; */
/*     } */
    
/*     CPE_INFO( */
/*         protocol->m_em, "ssl: %s: generate cert: %s", */
/*         net_protocol_name(base_protocol), */
/*         net_ssl_dump_cert_info(net_schedule_tmp_buffer(schedule), x509)); */
        
    return 0;

PROCESS_ERROR:
    mbedtls_x509_crt_free(protocol->m_svr.m_cert);
    mem_free(protocol->m_alloc, protocol->m_svr.m_cert);
    protocol->m_svr.m_cert = NULL;
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
