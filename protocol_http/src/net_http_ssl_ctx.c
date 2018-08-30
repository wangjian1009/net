#include "cpe/pal/pal_string.h"
#include "net_endpoint.h"
#include "net_http_ssl_ctx_i.h"

static void net_http_ssl_ctx_log(void *ctx, int level, const char *file, int line, const char *str);
static int net_http_ssl_ctx_recv(void *ctx, unsigned char *buf, size_t len);
static int net_http_ssl_ctx_send(void *ctx, const unsigned char *buf, size_t len);

net_http_ssl_ctx_t net_http_ssl_ctx_create(net_http_endpoint_t http_ep) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    net_http_ssl_ctx_t ssl_ctx = TAILQ_FIRST(&http_protocol->m_free_ssl_ctxes);
    if (ssl_ctx) {
        TAILQ_REMOVE(&http_protocol->m_free_ssl_ctxes, ssl_ctx, m_next);
    }
    else {
        ssl_ctx = mem_alloc(http_protocol->m_alloc, sizeof(struct net_http_ssl_ctx));
        if (ssl_ctx == NULL) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: create ssl ctx: alloc fail!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            return NULL;
        }
    }

    ssl_ctx->m_http_ep = http_ep;
    mbedtls_entropy_init(&ssl_ctx->m_entropy);
    mbedtls_ctr_drbg_init(&ssl_ctx->m_ctr_drbg);
    mbedtls_ssl_config_init(&ssl_ctx->m_conf);
    mbedtls_ssl_init(&ssl_ctx->m_ssl );
    mbedtls_x509_crt_init(&ssl_ctx->m_cacert);

    int rv;

    const char * pers = "test-pers";
    rv = mbedtls_ctr_drbg_seed(&ssl_ctx->m_ctr_drbg, mbedtls_entropy_func, &ssl_ctx->m_entropy, (const unsigned char *)pers, strlen(pers));
    if(rv != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: create ssl ctx: drbg_seed fail, rv=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), rv);
        goto INIT_ERROR;
    }
    
    rv = mbedtls_ssl_config_defaults(&ssl_ctx->m_conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if(rv != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: create ssl ctx: config defaults fail, rv=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), rv);
        goto INIT_ERROR;
    }

    mbedtls_ssl_conf_authmode(&ssl_ctx->m_conf, MBEDTLS_SSL_VERIFY_NONE);
    /* mbedtls_ssl_conf_authmode(&ssl_ctx->m_conf, MBEDTLS_SSL_VERIFY_OPTIONAL); */
    /* mbedtls_ssl_conf_ca_chain(&ssl_ctx->m_conf, &ssl_ctx->m_cacert, NULL); */
    mbedtls_ssl_conf_rng(&ssl_ctx->m_conf, mbedtls_ctr_drbg_random, &ssl_ctx->m_ctr_drbg);
    mbedtls_ssl_conf_dbg(&ssl_ctx->m_conf, net_http_ssl_ctx_log, ssl_ctx);

    if((rv = mbedtls_ssl_setup(&ssl_ctx->m_ssl, &ssl_ctx->m_conf)) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: create ssl ctx: ssl setup fail, rv=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), rv);
        goto INIT_ERROR;
    }

    const char * hostname = "localhost";
    if((rv = mbedtls_ssl_set_hostname(&ssl_ctx->m_ssl, hostname) ) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: create ssl ctx: ssl set host name %s fail, rv=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), hostname, rv);
        goto INIT_ERROR;
    }
    mbedtls_ssl_set_bio(&ssl_ctx->m_ssl, ssl_ctx, net_http_ssl_ctx_send, net_http_ssl_ctx_recv, NULL);
    
    return ssl_ctx;

INIT_ERROR:
    mbedtls_ssl_free(&ssl_ctx->m_ssl );
    mbedtls_ssl_config_free(&ssl_ctx->m_conf);
    mbedtls_ctr_drbg_free(&ssl_ctx->m_ctr_drbg);
    mbedtls_entropy_free(&ssl_ctx->m_entropy);
    mbedtls_x509_crt_free(&ssl_ctx->m_cacert);
    
    ssl_ctx->m_http_ep = (net_http_endpoint_t)http_protocol;
    TAILQ_INSERT_TAIL(&http_protocol->m_free_ssl_ctxes, ssl_ctx, m_next);
    return NULL;
}

void net_http_ssl_ctx_free(net_http_ssl_ctx_t ssl_ctx) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(ssl_ctx->m_http_ep);

    mbedtls_ssl_free(&ssl_ctx->m_ssl );
    mbedtls_ssl_config_free(&ssl_ctx->m_conf);
    mbedtls_ctr_drbg_free(&ssl_ctx->m_ctr_drbg);
    mbedtls_entropy_free(&ssl_ctx->m_entropy);
    mbedtls_x509_crt_free(&ssl_ctx->m_cacert);
    
    ssl_ctx->m_http_ep = (net_http_endpoint_t)http_protocol;
    TAILQ_INSERT_TAIL(&http_protocol->m_free_ssl_ctxes, ssl_ctx, m_next);
}

void net_http_ssl_ctx_real_free(net_http_ssl_ctx_t ssl_ctx) {
    net_http_protocol_t http_protocol = (net_http_protocol_t)ssl_ctx->m_http_ep;
    TAILQ_REMOVE(&http_protocol->m_free_ssl_ctxes, ssl_ctx, m_next);
    mem_free(http_protocol->m_alloc, ssl_ctx);
}

static void net_http_ssl_ctx_log(void *ctx, int level, const char *file, int line, const char *str) {
    net_http_ssl_ctx_t ssl_ctx = ctx;
    net_http_endpoint_t http_ep = ssl_ctx->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 2) {
        CPE_INFO(
            http_protocol->m_em, "http: %s: ssl: %s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            str);
    }
}

//mbedtls_net_recv
static int net_http_ssl_ctx_recv(void *ctx, unsigned char *buf, size_t len) {
    net_http_ssl_ctx_t ssl_ctx = ctx;
    net_http_endpoint_t http_ep = ssl_ctx->m_http_ep;
    //net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    switch(net_endpoint_state(http_ep->m_endpoint)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        return MBEDTLS_ERR_SSL_WANT_READ;
    case net_endpoint_state_established:
        break;
    case net_endpoint_state_logic_error:
    case net_endpoint_state_network_error:
        return MBEDTLS_ERR_NET_RECV_FAILED;
    }
        
    uint32_t sz;
    void * i_data = net_endpoint_buf_peak(http_ep->m_endpoint, net_ep_buf_read, &sz);
    if (i_data == NULL) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    if (sz > len) sz = (uint32_t)len;

    memcpy(buf, i_data, sz);

    net_endpoint_buf_consume(http_ep->m_endpoint, net_ep_buf_read, sz);
    
    return (int)sz;
}

//mbedtls_net_send 
static int net_http_ssl_ctx_send(void *ctx, const unsigned char *buf, size_t len) {
    net_http_ssl_ctx_t ssl_ctx = ctx;
    net_http_endpoint_t http_ep = ssl_ctx->m_http_ep;
    //net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    switch(net_endpoint_state(http_ep->m_endpoint)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    case net_endpoint_state_established:
        break;
    case net_endpoint_state_logic_error:
    case net_endpoint_state_network_error:
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }

    if (net_endpoint_buf_append(http_ep->m_endpoint, net_ep_buf_write, buf, (uint32_t)len) != 0) {
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }
    
    return (int)len;
}
