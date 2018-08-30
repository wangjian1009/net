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
    mbedtls_ssl_init(&ssl_ctx->m_ssl);
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

void net_http_ssl_ctx_reset(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_ssl_ctx_t ssl_ctx) {
    mbedtls_ssl_free(&ssl_ctx->m_ssl);
    mbedtls_ssl_init(&ssl_ctx->m_ssl);
}

int net_http_ssl_ctx_setup(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_ssl_ctx_t ssl_ctx) {
    int rv;

    if((rv = mbedtls_ssl_setup(&ssl_ctx->m_ssl, &ssl_ctx->m_conf)) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: create ssl ctx: ssl setup fail, rv=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), rv);
        return -1;
    }

    const char * hostname = "localhost";
    if((rv = mbedtls_ssl_set_hostname(&ssl_ctx->m_ssl, hostname) ) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: create ssl ctx: ssl set host name %s fail, rv=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), hostname, rv);
        return -1;
    }
    mbedtls_ssl_set_bio(&ssl_ctx->m_ssl, ssl_ctx, net_http_ssl_ctx_send, net_http_ssl_ctx_recv, NULL);

    return 0;
}

static void net_http_ssl_ctx_log(void *ctx, int level, const char *file, int line, const char *str) {
    net_http_ssl_ctx_t ssl_ctx = ctx;
    net_http_endpoint_t http_ep = ssl_ctx->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    /* if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 2) { */
        CPE_INFO(
            http_protocol->m_em, "http: %s: ssl:                        %s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            str);
    /* } */
}

//mbedtls_net_recv
static int net_http_ssl_ctx_recv(void *ctx, unsigned char *buf, size_t len) {
    net_http_ssl_ctx_t ssl_ctx = ctx;
    net_http_endpoint_t http_ep = ssl_ctx->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    switch(net_endpoint_state(http_ep->m_endpoint)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        CPE_ERROR(
            http_protocol->m_em, "http: %s: ssl: recv in state %s, return %d(%s)",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            net_endpoint_state_str(net_endpoint_state(http_ep->m_endpoint)),
            MBEDTLS_ERR_NET_INVALID_CONTEXT, net_http_ssl_error_str(MBEDTLS_ERR_NET_INVALID_CONTEXT));
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        CPE_ERROR(
            http_protocol->m_em, "http: %s: ssl: recv in state %s, return %d(%s)",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            net_endpoint_state_str(net_endpoint_state(http_ep->m_endpoint)),
            MBEDTLS_ERR_SSL_WANT_READ, net_http_ssl_error_str(MBEDTLS_ERR_SSL_WANT_READ));
        return MBEDTLS_ERR_SSL_WANT_READ;
    case net_endpoint_state_established:
        break;
    case net_endpoint_state_logic_error:
    case net_endpoint_state_network_error:
        CPE_ERROR(
            http_protocol->m_em, "http: %s: ssl: recv in state %s, return %d(%s)",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            net_endpoint_state_str(net_endpoint_state(http_ep->m_endpoint)),
            MBEDTLS_ERR_NET_RECV_FAILED, net_http_ssl_error_str(MBEDTLS_ERR_NET_RECV_FAILED));
        return MBEDTLS_ERR_NET_RECV_FAILED;
    }
        
    uint32_t sz;
    void * i_data = net_endpoint_buf_peak(http_ep->m_endpoint, net_ep_buf_read, &sz);
    if (i_data == NULL) {
        if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 3) {
            CPE_INFO(
                http_protocol->m_em, "http: %s: ssl: recv no data, return %d(%s)",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                MBEDTLS_ERR_SSL_WANT_READ, net_http_ssl_error_str(MBEDTLS_ERR_SSL_WANT_READ));
        }
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    if (sz > len) sz = (uint32_t)len;
    
    memcpy(buf, i_data, sz);

    net_endpoint_buf_consume(http_ep->m_endpoint, net_ep_buf_read, sz);

    if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 3) {
        CPE_INFO(
            http_protocol->m_em, "http: %s: <<< ssl %d data",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            sz);
    }
    
    return (int)sz;
}

//mbedtls_net_send 
static int net_http_ssl_ctx_send(void *ctx, const unsigned char *buf, size_t len) {
    net_http_ssl_ctx_t ssl_ctx = ctx;
    net_http_endpoint_t http_ep = ssl_ctx->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);
    
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

    if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 3) {
        CPE_INFO(
            http_protocol->m_em, "http: %s: ssl: >>> %d data",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            (int)len);
    }
    
    return (int)len;
}

const char * net_http_ssl_error_str(int rv) {
    switch(rv) {
    case MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE:
        return "ssl-feature-unavailable";
    case MBEDTLS_ERR_SSL_BAD_INPUT_DATA:
        return "ssl-bad_input_data";
    case MBEDTLS_ERR_SSL_INVALID_MAC:
        return "ssl-invalid-mac";
    case MBEDTLS_ERR_SSL_INVALID_RECORD:
        return "ssl-invalid-record";
    case MBEDTLS_ERR_SSL_CONN_EOF:
        return "ssl-conn-eof";
    case MBEDTLS_ERR_SSL_UNKNOWN_CIPHER:
        return "ssl-unknown-cipher";
    case MBEDTLS_ERR_SSL_NO_CIPHER_CHOSEN:
        return "ssl-no-cipher-chosen";
    case MBEDTLS_ERR_SSL_NO_RNG:
        return "ssl-no-rng";
    case MBEDTLS_ERR_SSL_NO_CLIENT_CERTIFICATE:
        return "ssl-no-client-certificate";
    case MBEDTLS_ERR_SSL_CERTIFICATE_TOO_LARGE:
        return "ssl-certificate-too-large";
    case MBEDTLS_ERR_SSL_CERTIFICATE_REQUIRED:
        return "ssl-certificate-required";
    case MBEDTLS_ERR_SSL_PRIVATE_KEY_REQUIRED:
        return "ssl-private-key-required";
    case MBEDTLS_ERR_SSL_CA_CHAIN_REQUIRED:
        return "ssl-ca-chain-required";
    case MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE:
        return "ssl-unexpected-message";
    case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE:
        return "ssl-fatal-alert-message";
    case MBEDTLS_ERR_SSL_PEER_VERIFY_FAILED:
        return "ssl-peer-verify-failed";
    case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
        return "ssl-peer-close-notify";
    case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO:
        return "ssl-bad-hs-client-hello";
    case MBEDTLS_ERR_SSL_BAD_HS_SERVER_HELLO:
        return "ssl-bad-hs-server-hello";
    case MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE:
        return "ssl-bad-hs-certificate";
    case MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_REQUEST:
        return "ssl-BAD_HS_CERTIFICATE_REQUEST";
    case MBEDTLS_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE:
        return "ssl-bad-hs-server-key-exchange";
    case MBEDTLS_ERR_SSL_BAD_HS_SERVER_HELLO_DONE:
        return "ssl-bad-hs-server-hello-done";
    case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE:
        return "ssl-bad-hs-client-key-exchange";
    case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_RP:
        return "ssl-bad-hs-client-key-exchange-rp";
    case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_CS:
        return "ssl-bad-hs-client-key-exchange-cs";
    case MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY:
        return "ssl-bad-hs-certificate-verify";
    case MBEDTLS_ERR_SSL_BAD_HS_CHANGE_CIPHER_SPEC:
        return "ssl-bad-hs-change-cipher-spec";
    case MBEDTLS_ERR_SSL_BAD_HS_FINISHED:
        return "ssl-bad-hs-finished";
    case MBEDTLS_ERR_SSL_ALLOC_FAILED:
        return "ssl-alloc-failed";
    case MBEDTLS_ERR_SSL_HW_ACCEL_FAILED:
        return "ssl-hw-accel-failed";
    case MBEDTLS_ERR_SSL_HW_ACCEL_FALLTHROUGH:
        return "ssl-hw-accel-fallthrough";
    case MBEDTLS_ERR_SSL_COMPRESSION_FAILED:
        return "ssl-compression-failed";
    case MBEDTLS_ERR_SSL_BAD_HS_PROTOCOL_VERSION:
        return "ssl-bad-hs-protocol-version";
    case MBEDTLS_ERR_SSL_BAD_HS_NEW_SESSION_TICKET:
        return "ssl-bad-hs-new-session-ticket";
    case MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED:
        return "ssl-session-ticket-expired";
    case MBEDTLS_ERR_SSL_PK_TYPE_MISMATCH:
        return "ssl-pk-type-mismatch";
    case MBEDTLS_ERR_SSL_UNKNOWN_IDENTITY:
        return "ssl-unknown-identity";
    case MBEDTLS_ERR_SSL_INTERNAL_ERROR:
        return "ssl-internal-error";
    case MBEDTLS_ERR_SSL_COUNTER_WRAPPING:
        return "ssl-counter-wrapping";
    case MBEDTLS_ERR_SSL_WAITING_SERVER_HELLO_RENEGO:
        return "ssl-waiting-server-hello-renego";
    case MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED:
        return "ssl-hello-verify-required";
    case MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL:
        return "ssl-buffer-too-small";
    case MBEDTLS_ERR_SSL_NO_USABLE_CIPHERSUITE:
        return "ssl-no-usable-ciphersuite";
    case MBEDTLS_ERR_SSL_WANT_READ:
        return "ssl-want-read";
    case MBEDTLS_ERR_SSL_WANT_WRITE:
        return "ssl-want-write";
    case MBEDTLS_ERR_SSL_TIMEOUT:
        return "ssl-timeout";
    case MBEDTLS_ERR_SSL_CLIENT_RECONNECT:
        return "ssl-client-reconnect";
    case MBEDTLS_ERR_SSL_UNEXPECTED_RECORD:
        return "ssl-unexpected-record";
    case MBEDTLS_ERR_SSL_NON_FATAL:
        return "ssl-non-fatal";
    case MBEDTLS_ERR_SSL_INVALID_VERIFY_HASH:
        return "ssl-invalid-verify-hash";
    case MBEDTLS_ERR_SSL_CONTINUE_PROCESSING:
        return "ssl-continue-processing";
    case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
        return "ssl-async-in-progress";
    default:
        return "ssl-unknown-error";
    }
}
