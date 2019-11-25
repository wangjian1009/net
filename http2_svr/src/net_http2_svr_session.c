#include "cpe/pal/pal_strings.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_http2_svr_session_i.h"
#include "net_http2_svr_request_i.h"

static net_http2_svr_request_t net_http2_svr_session_new_request(void *data);

ssize_t net_http2_svr_send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data);
int net_http2_svr_on_frame_recv_callback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);
int net_http2_svr_on_stream_close_callback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data);
int net_http2_svr_on_header_callback(
    nghttp2_session *session,
    const nghttp2_frame *frame, const uint8_t *name,
    size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data);
int net_http2_svr_on_begin_headers_callback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);

int net_http2_svr_session_init(net_endpoint_t base_endpoint) {
    net_http2_svr_service_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_http2_svr_session_t session = net_endpoint_data(base_endpoint);

    TAILQ_INIT(&session->m_requests);
    session->m_session = NULL;

    /* const unsigned char *alpn = NULL; */
    /* unsigned int alpnlen = 0; */
    /* SSL *ssl; */
    //(void)bev;

    //CPE_INFO(service->m_em, "%s connected\n", session_data->client_addr);

/*     ssl = bufferevent_openssl_get_ssl(session_data->bev); */

/* #ifndef OPENSSL_NO_NEXTPROTONEG */
/*     SSL_get0_next_proto_negotiated(ssl, &alpn, &alpnlen); */
/* #endif /\* !OPENSSL_NO_NEXTPROTONEG *\/ */
/* #if OPENSSL_VERSION_NUMBER >= 0x10002000L */
/*     if (alpn == NULL) { */
/*       SSL_get0_alpn_selected(ssl, &alpn, &alpnlen); */
/*     } */
/* #endif /\* OPENSSL_VERSION_NUMBER >= 0x10002000L *\/ */

    /* if (alpn == NULL || alpnlen != 2 || memcmp("h2", alpn, 2) != 0) { */
    /*   fprintf(stderr, "%s h2 is not negotiated\n", session_data->client_addr); */
    /*   delete_http2_session_data(session_data); */
    /*   return; */
    /* } */

    nghttp2_session_callbacks * callbacks = NULL;
    if (nghttp2_session_callbacks_new(&callbacks) != 0) {
        CPE_ERROR(
            service->m_em, "nghttp2: %s: create callbacks fail!",
            net_endpoint_dump(net_http2_svr_service_tmp_buffer(service), base_endpoint));
        return -1;
    }

    nghttp2_session_callbacks_set_send_callback(callbacks, net_http2_svr_send_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, net_http2_svr_on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, net_http2_svr_on_stream_close_callback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, net_http2_svr_on_header_callback);
    nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, net_http2_svr_on_begin_headers_callback);
    if (nghttp2_session_server_new(&session->m_session, callbacks, base_endpoint) != 0) {
        CPE_ERROR(
            service->m_em, "nghttp2: %s: create session fail!",
            net_endpoint_dump(net_http2_svr_service_tmp_buffer(service), base_endpoint));
        nghttp2_session_callbacks_del(callbacks);
        return -1;
    }
    nghttp2_session_callbacks_del(callbacks);

    /* if (send_server_connection_header(session_data) != 0 || */
    /*     session_send(session_data) != 0) { */
    /*   delete_http2_session_data(session_data); */
    /*   return; */
    /* } */

    return 0;
}

void net_http2_svr_session_fini(net_endpoint_t base_endpoint) {
    //net_http2_svr_service_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_http2_svr_session_t session = net_endpoint_data(base_endpoint);

    while(!TAILQ_EMPTY(&session->m_requests)) {
        net_http2_svr_request_free(TAILQ_FIRST(&session->m_requests));
    }
}

int net_http2_svr_session_input(net_endpoint_t base_endpoint) {
    net_http2_svr_service_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_http2_svr_session_t session = net_endpoint_data(base_endpoint);

    net_http2_svr_session_timeout_reset(session);

    while(net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        uint32_t data_size = 0;
        void* data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_read, &data_size);
        if (data == NULL || data_size == 0) return 0;

        //net_http2_svr_request_parser_execute(&session->parser, data, data_size);

        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return -1;
        
        net_endpoint_buf_consume(base_endpoint, net_ep_buf_read, data_size);
        
        /* parse error? just drop the client. screw the 400 response */
        /* if (net_http2_svr_request_parser_has_error(&session->parser)) { */
        /*     CPE_ERROR( */
        /*         service->m_em, "nghttp2: %s: request parser has error, auto close!", */
        /*         net_endpoint_dump(net_http2_svr_service_tmp_buffer(service), base_endpoint)); */
        /*     return -1; */
        /* } */
    }

    return -1;
}

net_http2_svr_service_t net_http2_svr_session_service(net_http2_svr_session_t session) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(session);
    return net_protocol_data(net_endpoint_protocol(base_endpoint));
}

static net_http2_svr_request_t net_http2_svr_session_new_request(void *data) {
    net_http2_svr_session_t session = data;
    //return net_http2_svr_request_create(session);
    return NULL;
}

void net_http2_svr_session_check_remove_done_requests(net_http2_svr_session_t session) {
    net_http2_svr_request_t request = TAILQ_FIRST(&session->m_requests);

    while(request && request->m_state == net_http2_svr_request_state_complete) {
        net_http2_svr_request_free(request);

        request = TAILQ_FIRST(&session->m_requests);
        if (request) {
        }
    }
}

ssize_t net_http2_svr_send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data) {
    /* http2_session_data* session_data = (http2_session_data*)user_data; */
    /* struct bufferevent* bev = session_data->bev; */
    /* (void)session; */
    /* (void)flags; */

    /* /\* Avoid excessive buffering in server side. *\/ */
    /* if (evbuffer_get_length(bufferevent_get_output(session_data->bev)) >= OUTPUT_WOULDBLOCK_THRESHOLD) { */
    /*     return NGHTTP2_ERR_WOULDBLOCK; */
    /* } */
    /* bufferevent_write(bev, data, length); */
    /* return (ssize_t)length; */
    return 0;
}

int net_http2_svr_on_frame_recv_callback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data) {
    /* http2_session_data* session_data = (http2_session_data*)user_data; */
    /* http2_stream_data* stream_data; */
    /* switch (frame->hd.type) { */
    /* case NGHTTP2_DATA: */
    /* case NGHTTP2_HEADERS: */
    /*     /\* Check that the client request has finished *\/ */
    /*     if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) { */
    /*         stream_data = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id); */
    /*         /\* For DATA and HEADERS frame, this callback may be called after */
    /*      on_stream_close_callback. Check that stream still alive. *\/ */
    /*         if (!stream_data) { */
    /*             return 0; */
    /*         } */
    /*         return on_request_recv(session, session_data, stream_data); */
    /*     } */
    /*     break; */
    /* default: */
    /*     break; */
    /* } */
    return 0;
}

int net_http2_svr_on_stream_close_callback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data) {
    /* http2_session_data* session_data = (http2_session_data*)user_data; */
    /* http2_stream_data* stream_data; */
    /* (void)error_code; */

    /* stream_data = nghttp2_session_get_stream_user_data(session, stream_id); */
    /* if (!stream_data) { */
    /*     return 0; */
    /* } */
    /* remove_stream(session_data, stream_data); */
    /* delete_http2_stream_data(stream_data); */
    return 0;
}

int net_http2_svr_on_header_callback(
    nghttp2_session *session,
    const nghttp2_frame *frame, const uint8_t *name,
    size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data)
{
    /* http2_stream_data* stream_data; */
    /* const char PATH[] = ":path"; */
    /* (void)flags; */
    /* (void)user_data; */

    /* switch (frame->hd.type) { */
    /* case NGHTTP2_HEADERS: */
    /*     if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) { */
    /*         break; */
    /*     } */
    /*     stream_data = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id); */
    /*     if (!stream_data || stream_data->request_path) { */
    /*         break; */
    /*     } */
    /*     if (namelen == sizeof(PATH) - 1 && memcmp(PATH, name, namelen) == 0) { */
    /*         size_t j; */
    /*         for (j = 0; j < valuelen && value[j] != '?'; ++j) */
    /*             ; */
    /*         stream_data->request_path = percent_decode(value, j); */
    /*     } */
    /*     break; */
    /* } */
    return 0;
}

int net_http2_svr_on_begin_headers_callback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data) {
    /* http2_session_data* session_data = (http2_session_data*)user_data; */
    /* http2_stream_data* stream_data; */

    /* if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) { */
    /*     return 0; */
    /* } */
    /* stream_data = create_http2_stream_data(session_data, frame->hd.stream_id); */
    /* nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, stream_data); */
    return 0;
}
