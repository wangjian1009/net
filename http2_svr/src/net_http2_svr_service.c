#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_http2_svr_service_i.h"
#include "net_http2_svr_session_i.h"
#include "net_http2_svr_request_i.h"
#include "net_http2_svr_request_header_i.h"
#include "net_http2_svr_mount_point_i.h"
#include "net_http2_svr_processor_i.h"
#include "net_http2_svr_response_i.h"

static int net_http2_svr_service_init(net_protocol_t protocol);
static void net_http2_svr_service_fini(net_protocol_t protocol);
int net_http2_svr_service_next_proto_cb(SSL *ssl, const unsigned char **data, unsigned int *len, void *arg);
int net_http2_svr_service_alpn_select_proto_cb(
    SSL* ssl, const unsigned char** out, unsigned char* outlen, const unsigned char* in, unsigned int inlen, void* arg);

net_http2_svr_service_t
net_http2_svr_service_create(net_schedule_t schedule, const char * protocol_name) {
    net_protocol_t protocol =
        net_protocol_create(
            schedule,
            protocol_name,
            /*protocol*/
            sizeof(struct net_http2_svr_service),
            net_http2_svr_service_init,
            net_http2_svr_service_fini,
            /*endpoint*/
            sizeof(struct net_http2_svr_session),
            net_http2_svr_session_init,
            net_http2_svr_session_fini,
            net_http2_svr_session_input,
            NULL,
            NULL,
            NULL);
    if (protocol == NULL) {
        CPE_ERROR(net_schedule_em(schedule), "nghttp2: %s: create protocol fail!", protocol_name);
        return NULL;
    }
    net_protocol_set_debug(protocol, 2);

    net_http2_svr_service_t service = net_protocol_data(protocol);
    error_monitor_t em = net_schedule_em(schedule);

    service->m_ssl_ctx = SSL_CTX_new(SSLv23_server_method());
    if (service->m_ssl_ctx == NULL) {
        CPE_ERROR(em, "nghttp2: %s: Could not create SSL/TLS context: %s", protocol_name, ERR_error_string(ERR_get_error(), NULL));
        goto INIT_ERROR;
    }

    SSL_CTX_set_options(
        service->m_ssl_ctx,
        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
        SSL_OP_NO_COMPRESSION |
        SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

  EC_KEY * ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  if (ecdh == NULL) {
      CPE_ERROR(em, "nghttp2: %s: EC_KEY_new_by_curv_name failed: %s", protocol_name, ERR_error_string(ERR_get_error(), NULL));
      goto INIT_ERROR;
  }
  SSL_CTX_set_tmp_ecdh(service->m_ssl_ctx, ecdh);
  EC_KEY_free(ecdh);

  /* if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM) != 1) { */
  /*     //errx(1, "Could not read private key file %s", key_file); */
  /* } */
  
  /* if (SSL_CTX_use_certificate_chain_file(ssl_ctx, cert_file) != 1) { */
  /*   errx(1, "Could not read certificate file %s", cert_file); */
  /* } */

  SSL_CTX_set_next_protos_advertised_cb(service->m_ssl_ctx, net_http2_svr_service_next_proto_cb, service);
  SSL_CTX_set_alpn_select_cb(service->m_ssl_ctx, net_http2_svr_service_alpn_select_proto_cb, service);

  return service;

INIT_ERROR:
    net_protocol_free(protocol);
    return NULL;
}

void net_http2_svr_service_free(net_http2_svr_service_t service) {
    net_protocol_free(net_protocol_from_data(service));
}

net_protocol_t net_http2_svr_service_to_protocol(net_http2_svr_service_t service) {
    return net_protocol_from_data(service);
}

net_http2_svr_mount_point_t net_http2_svr_service_root(net_http2_svr_service_t service) {
    return service->m_root;
}

mem_buffer_t net_http2_svr_service_tmp_buffer(net_http2_svr_service_t service) {
    net_protocol_t protocol = net_http2_svr_service_to_protocol(service);
    return net_schedule_tmp_buffer(net_protocol_schedule(protocol));
}

static int net_http2_svr_service_init(net_protocol_t protocol) {
    net_http2_svr_service_t service = net_protocol_data(protocol);

    service->m_em = net_schedule_em(net_protocol_schedule(protocol));
    service->m_alloc = net_schedule_allocrator(net_protocol_schedule(protocol));
    service->m_cfg_session_timeout_ms = 30 * 1000;
    service->m_ssl_ctx = NULL;
    service->m_next_proto_list[0] = NGHTTP2_PROTO_VERSION_ID_LEN;
    memcpy(&service->m_next_proto_list[1], NGHTTP2_PROTO_VERSION_ID, NGHTTP2_PROTO_VERSION_ID_LEN);
    service->m_next_proto_list_len = 1 + NGHTTP2_PROTO_VERSION_ID_LEN;

    service->m_next_proto_list[0] = NGHTTP2_PROTO_VERSION_ID_LEN;

    TAILQ_INIT(&service->m_processors);
    service->m_root = NULL;
    TAILQ_INIT(&service->m_sessions);
    service->m_request_sz = 0;
    
    service->m_max_request_id = 0;
    TAILQ_INIT(&service->m_free_requests);
    TAILQ_INIT(&service->m_free_request_headers);
    TAILQ_INIT(&service->m_free_mount_points);
    TAILQ_INIT(&service->m_free_responses);

    mem_buffer_init(&service->m_data_buffer, NULL);
    mem_buffer_init(&service->m_search_path_buffer, NULL);

    net_http2_svr_processor_t dft_processor = net_http2_svr_processor_create_dft(service);
    if (dft_processor == NULL) {
        CPE_ERROR(service->m_em, "nghttp2: %s: create dft processor fail!", net_protocol_name(protocol));
        return -1;
    }
    
    const char * root_mp_name = "root";
    service->m_root = net_http2_svr_mount_point_create(
        service, root_mp_name, root_mp_name + strlen(root_mp_name), NULL, NULL, dft_processor);
    if (service->m_root == NULL) {
        CPE_ERROR(service->m_em, "nghttp2: %s: create root mount point fail!", net_protocol_name(protocol));
        net_http2_svr_processor_free(dft_processor);
        return -1;
    }
    
    return 0;
}

static void net_http2_svr_service_fini(net_protocol_t protocol) {
    net_http2_svr_service_t service = net_protocol_data(protocol);
    assert(TAILQ_EMPTY(&service->m_sessions));

    if (service->m_root) {
        net_http2_svr_mount_point_free(service->m_root);
        service->m_root = NULL;
    }
    
    while(!TAILQ_EMPTY(&service->m_processors)) {
        net_http2_svr_processor_free(TAILQ_FIRST(&service->m_processors));
    }

    if (service->m_ssl_ctx) {
        SSL_CTX_free(service->m_ssl_ctx);
        service->m_ssl_ctx = NULL;
    }

    while(!TAILQ_EMPTY(&service->m_free_requests)) {
        net_http2_svr_request_real_free(TAILQ_FIRST(&service->m_free_requests));
    }

    while(!TAILQ_EMPTY(&service->m_free_request_headers)) {
        net_http2_svr_request_header_real_free(TAILQ_FIRST(&service->m_free_request_headers));
    }

    while(!TAILQ_EMPTY(&service->m_free_responses)) {
        net_http2_svr_response_real_free(TAILQ_FIRST(&service->m_free_responses));
    }
    
    while (!TAILQ_EMPTY(&service->m_free_mount_points)) {
        net_http2_svr_mount_point_real_free(TAILQ_FIRST(&service->m_free_mount_points));
    }

    mem_buffer_clear(&service->m_data_buffer);
    mem_buffer_clear(&service->m_search_path_buffer);
}

struct {
    const char * m_mine_type;
    const char * m_postfix;
} s_common_mine_type_defs[] = {
    { "application/msword", "doc" }
    , { "application/pdf", "pdf" }
    , { "application/rtf", "rtf" }
    , { "application/vnd.ms-excel", "xls" }
    , { "application/vnd.ms-powerpoint", "ppt" }
    , { "application/x-rar-compressed", "rar" }
    , { "application/x-shockwave-flash", "swf" }
    , { "application/zip", "zip" }
    , { "audio/midi", "mid" }     //midi kar
    , { "audio/mpeg", "mp3" }
    , { "audio/ogg", "ogg" }
    , { "audio/x-m4a", "m4a" }
    , { "audio/x-realaudio", "ra" }
    , { "image/gif", "gif" }
    , { "image/jpeg", "jpeg" }
    , { "image/jpeg", "jpg" }
    , { "image/png", "png" }
    , { "image/tiff", "tif" }
    , { "image/tiff", "tiff" }
    , { "image/vnd.wap.wbmp", "wbmp" }
    , { "image/x-icon", "ico" }
    , { "image/x-jng", "jng" }
    , { "image/x-ms-bmp", "bmp" }
    , { "image/svg+xml", "svg" }
    , { "image/svg+xml", "svgz" }
    , { "image/webp", "webp" }
    , { "text/css", "css" }
    , { "text/html", "html" }
    , { "text/html", "tml" }
    , { "text/html", "shtml" }
    , { "text/plain", "txt" }
    , { "text/xml", "xml" }
    , { "video/3gpp", "3gpp" }
    , { "video/3gpp", "3gp" }
    , { "video/mp4", "mp4" }
    , { "video/mpeg", "mpeg" }
    , { "video/mpeg", "mpg" }
    , { "video/quicktime", "mov" }
    , { "video/webm", "webm" }
    , { "video/x-flv", "flv" }
    , { "video/x-m4v", "m4v" }
    , { "video/x-ms-wmv", "wmv" }
    , { "video/x-msvideo", "avi" }
};

const char * net_http2_svr_service_mine_from_postfix(net_http2_svr_service_t service, const char * postfix) {
    uint16_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(s_common_mine_type_defs); ++i) {
        if (strcasecmp(postfix, s_common_mine_type_defs[i].m_postfix) == 0) {
            return s_common_mine_type_defs[i].m_mine_type;
        }
    }
    
    return NULL;
}

int net_http2_svr_service_next_proto_cb(SSL* ssl, const unsigned char** data, unsigned int* len, void* arg) {
    net_http2_svr_service_t service = arg;
    (void)ssl;

    *data = service->m_next_proto_list;
    *len = service->m_next_proto_list_len;
    return SSL_TLSEXT_ERR_OK;
}

int net_http2_svr_service_alpn_select_proto_cb(
    SSL* ssl, const unsigned char** out, unsigned char* outlen, const unsigned char* in, unsigned int inlen, void* arg)
{
    net_http2_svr_service_t service = arg;
    (void)ssl;

    int rv = nghttp2_select_next_protocol((unsigned char**)out, outlen, in, inlen);

    if (rv != 1) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    return SSL_TLSEXT_ERR_OK;
}
