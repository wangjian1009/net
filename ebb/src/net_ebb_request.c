#include "cpe/utils/stream_buffer.h"
#include "net_endpoint.h"
#include "net_ebb_request_i.h"

static void net_ebb_request_on_path(net_ebb_request_t, const char* at, size_t length);
static void net_ebb_request_on_query_string(net_ebb_request_t, const char* at, size_t length);
static void net_ebb_request_on_uri(net_ebb_request_t, const char* at, size_t length);
static void net_ebb_request_on_fragment(net_ebb_request_t, const char* at, size_t length);
static void net_ebb_request_on_head_fileld(net_ebb_request_t, const char* at, size_t length, int header_index);
static void net_ebb_request_on_head_value(net_ebb_request_t, const char* at, size_t length, int header_index);

net_ebb_request_t net_ebb_request_create(net_ebb_connection_t connection) {
    net_ebb_service_t service = net_ebb_connection_service(connection);

    net_ebb_request_t request = TAILQ_FIRST(&service->m_free_requests);
    if (request) {
        TAILQ_REMOVE(&service->m_free_requests, request, m_next);
    }
    else {
        request = mem_alloc(service->m_alloc, sizeof(struct net_ebb_request));
        if (request == NULL) {
            CPE_ERROR(
                service->m_em, "ebb: %s: request: alloc fail!",
                net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)));
            return NULL;
        }
    }

    request->m_connection = connection;
    request->m_method = net_ebb_request_method_unknown;
    request->m_expect_continue = 0;
    request->m_body_read = 0;
    request->m_content_length = 0;
    request->m_version_major = 0;
    request->m_version_minor = 0;
    request->m_number_of_headers = 0;
    request->m_transfer_encoding = net_ebb_request_transfer_encoding_identity;
    request->m_keep_alive = -1;

    request->on_complete = NULL;
    request->on_headers_complete = NULL;
    request->on_body = NULL;
    request->on_header_field = net_ebb_request_on_head_fileld;
    request->on_header_value = net_ebb_request_on_head_value;
    request->on_uri = net_ebb_request_on_uri;
    request->on_fragment = net_ebb_request_on_fragment;
    request->on_path = net_ebb_request_on_path;
    request->on_query_string = net_ebb_request_on_query_string;
    
    TAILQ_INSERT_TAIL(&connection->m_requests, request, m_next);
    
    return request;
}

void net_ebb_request_free(net_ebb_request_t request) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    
    TAILQ_REMOVE(&connection->m_requests, request, m_next);
    
    request->m_connection = (net_ebb_connection_t)service;
    TAILQ_INSERT_TAIL(&service->m_free_requests, request, m_next);
}

void net_ebb_request_real_free(net_ebb_request_t request) {
    net_ebb_service_t service = (net_ebb_service_t)request->m_connection;
    
    TAILQ_REMOVE(&service->m_free_requests, request, m_next);
    
    mem_free(service->m_alloc, request);
}

net_ebb_request_method_t net_ebb_request_method(net_ebb_request_t request) {
    return request->m_method;
}

net_ebb_request_transfer_encoding_t net_ebb_request_transfer_encoding(net_ebb_request_t request) {
    return request->m_transfer_encoding;
}

uint8_t net_ebb_request_expect_continue(net_ebb_request_t request) {
    return request->m_expect_continue;
}

uint16_t net_ebb_request_version_major(net_ebb_request_t request) {
    return request->m_version_major;
}

uint16_t net_ebb_request_version_minor(net_ebb_request_t request) {
    return request->m_version_minor;
}

uint8_t net_ebb_request_should_keep_alive(net_ebb_request_t request) {
    if (request->m_keep_alive == -1) {
        if (request->m_version_major == 1) {
            return (request->m_version_minor != 0);
        }
        else if (request->m_version_major == 0) {
            return 0;
        }
        else {
            return 1;
        }
    }
    else {
        return request->m_keep_alive;
    }
}

void net_ebb_request_print(write_stream_t ws, net_ebb_request_t request) {
    stream_printf(ws, "%s %d.%d", net_ebb_request_method_str(request->m_method), request->m_version_major, request->m_version_minor);
}

const char * net_ebb_request_dump(mem_buffer_t buffer, net_ebb_request_t request) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);
    net_ebb_request_print((write_stream_t)&stream, request);
    stream_putc((write_stream_t)&stream, 0);
    return mem_buffer_make_continuous(buffer, 0);
}

static void net_ebb_request_on_path(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on path: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
    }
}

static void net_ebb_request_on_query_string(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on query string: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
    }
}

static void net_ebb_request_on_uri(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on uri: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
    }
}

static void net_ebb_request_on_fragment(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on fragment: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
    }
}

static void net_ebb_request_on_head_fileld(net_ebb_request_t request, const char* at, size_t length, int header_index) {
}

static void net_ebb_request_on_head_value(net_ebb_request_t request, const char* at, size_t length, int header_index) {
}

const char * net_ebb_request_method_str(net_ebb_request_method_t method) {
    switch(method) {
    case net_ebb_request_method_unknown:
        return "unknown";
    case net_ebb_request_method_copy:
        return "copy";
    case net_ebb_request_method_delete:
        return "delete";
    case net_ebb_request_method_get:
        return "get";
    case net_ebb_request_method_head:
        return "head";
    case net_ebb_request_method_lock:
        return "lock";
    case net_ebb_request_method_mkcol:
        return "mkcol";
    case net_ebb_request_method_move:
        return "move";
    case net_ebb_request_method_options:
        return "options";
    case net_ebb_request_method_post:
        return "post";
    case net_ebb_request_method_propfind:
        return "propfind";
    case net_ebb_request_method_proppatch:
        return "proppatch";
    case net_ebb_request_method_put:
        return "put";
    case net_ebb_request_method_trace:
        return "trace";
    case net_ebb_request_method_unlock:
        return "unlock";
    }
}
