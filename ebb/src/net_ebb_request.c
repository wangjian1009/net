#include <assert.h>
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_ebb_request_i.h"
#include "net_ebb_request_header_i.h"
#include "net_ebb_mount_point_i.h"
#include "net_ebb_processor_i.h"
#include "net_ebb_response_i.h"

net_ebb_request_t net_ebb_request_create(net_ebb_connection_t connection) {
    net_ebb_service_t service = net_ebb_connection_service(connection);

    net_ebb_request_t request = TAILQ_FIRST(&service->m_free_requests);
    if (request) {
        TAILQ_REMOVE(&service->m_free_requests, request, m_next_for_connection);
    }
    else {
        request = mem_alloc(service->m_alloc, sizeof(struct net_ebb_request) + service->m_request_sz);
        if (request == NULL) {
            CPE_ERROR(
                service->m_em, "ebb: %s: request: alloc fail!",
                net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)));
            return NULL;
        }
    }

    request->m_connection = connection;
    request->m_processor = NULL;
    request->m_method = net_ebb_request_method_unknown;
    request->m_expect_continue = 0;
    request->m_body_read = 0;
    request->m_content_length = 0;
    request->m_version_major = 0;
    request->m_version_minor = 0;
    request->m_number_of_headers = 0;
    request->m_transfer_encoding = net_ebb_request_transfer_encoding_identity;
    request->m_keep_alive = -1;
    request->m_path = NULL;
    request->m_state = net_ebb_request_state_processing;
    request->m_response = NULL;
    TAILQ_INIT(&request->m_headers);

    TAILQ_INSERT_TAIL(&connection->m_requests, request, m_next_for_connection);
    
    return request;
}

void net_ebb_request_free(net_ebb_request_t request) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);

    net_ebb_request_set_processor(request, NULL);
    assert(request->m_processor == NULL);

    if (request->m_response) {
        net_ebb_response_free(request->m_response);
        request->m_response = NULL;
    }

    if (request->m_path) {
        mem_free(service->m_alloc, request->m_path);
        request->m_path = NULL;
    }

    while(!TAILQ_EMPTY(&request->m_headers)) {
        net_ebb_request_header_free(TAILQ_FIRST(&request->m_headers));
    }

    TAILQ_REMOVE(&connection->m_requests, request, m_next_for_connection);
    
    request->m_connection = (net_ebb_connection_t)service;
    TAILQ_INSERT_TAIL(&service->m_free_requests, request, m_next_for_connection);
}

void net_ebb_request_real_free(net_ebb_request_t request) {
    net_ebb_service_t service = (net_ebb_service_t)request->m_connection;
    
    TAILQ_REMOVE(&service->m_free_requests, request, m_next_for_connection);
    
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

const char * net_ebb_request_full_path(net_ebb_request_t request) {
    return request->m_path;
}

const char * net_ebb_request_relative_path(net_ebb_request_t request) {
    return request->m_path_to_processor;
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

void net_ebb_request_set_processor(net_ebb_request_t request, net_ebb_mount_point_t mp) {
    if (request->m_processor) {
        if (request->m_processor->m_request_fini) {
            request->m_processor->m_request_fini(request->m_processor->m_ctx, request);
        }
        TAILQ_REMOVE(&request->m_processor->m_requests, request, m_next_for_processor);
    }

    request->m_processor = mp ? mp->m_processor : NULL;

    if (request->m_processor) {
        if (request->m_processor->m_request_init) {
            request->m_processor->m_request_init(request->m_processor->m_ctx, mp->m_processor_env, request);
        }
        TAILQ_INSERT_TAIL(&request->m_processor->m_requests, request, m_next_for_processor);
    }
}

void net_ebb_request_print(write_stream_t ws, net_ebb_request_t request) {
    stream_printf(
        ws, "%s %d.%d", 
        net_ebb_request_method_str(request->m_method),
        request->m_version_major, request->m_version_minor);
    
    if (request->m_processor) {
        stream_printf(
            ws, " %s[%s]", 
            request->m_processor->m_name, request->m_path_to_processor);
    }
    
    stream_printf(ws, " (%s)", request->m_path);
    
    net_ebb_request_header_t header;
    TAILQ_FOREACH(header, &request->m_headers, m_next) {
        stream_printf(ws, "\n    %s: %s", header->m_name, header->m_value);
    }
}

const char * net_ebb_request_dump(mem_buffer_t buffer, net_ebb_request_t request) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);
    net_ebb_request_print((write_stream_t)&stream, request);
    stream_putc((write_stream_t)&stream, 0);
    return mem_buffer_make_continuous(buffer, 0);
}

void net_ebb_request_on_path(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    assert(request->m_path == NULL);
    
    request->m_path = cpe_str_mem_dup_len(service->m_alloc, at, length);
    if (request->m_path == NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: on path: dup path %.*s fail!", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
        net_ebb_connection_close_schedule(connection);
        return;
    }
    
    request->m_path_to_processor = request->m_path;

    net_ebb_mount_point_t mp = net_ebb_mount_point_find_by_path(service, &request->m_path_to_processor);
    if (mp == NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: on path: find processor at  path %s fail!", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            request->m_path);
        net_ebb_connection_close_schedule(connection);
        return;
    }

    net_ebb_request_set_processor(request, mp);
    
    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on path: bind to processor %s[%s] (full-path=%s)",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            request->m_processor->m_name, request->m_path_to_processor, request->m_path);
    }
}

void net_ebb_request_on_query_string(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    if (net_endpoint_protocol_debug(connection->m_endpoint) >= 2) {
        CPE_INFO(
            service->m_em, "ebb: %s: on query string: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
    }
}

void net_ebb_request_on_uri(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    if (net_endpoint_protocol_debug(connection->m_endpoint) >= 2) {
        CPE_INFO(
            service->m_em, "ebb: %s: on uri: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
    }
}

void net_ebb_request_on_fragment(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(connection->m_endpoint) >= 2) {
        CPE_INFO(
            service->m_em, "ebb: %s: on fragment: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
    }
}

void net_ebb_request_on_header_field(net_ebb_request_t request, const char* at, size_t length, int header_index) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(connection->m_endpoint) >= 2) {
        CPE_INFO(
            service->m_em, "ebb: %s: on header field %d: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            header_index, (int)length, at);
    }
    
    net_ebb_request_header_t header = net_ebb_request_header_create(request, (uint16_t)header_index, at, length);
    if (header == NULL) {
        CPE_INFO(
            service->m_em, "ebb: %s: header [%d]%.*s: create header fail",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            header_index, (int)length, at);
    }
}

void net_ebb_request_on_header_value(net_ebb_request_t request, const char* at, size_t length, int header_index) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on header value %d: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            header_index, (int)length, at);
    }
    
    net_ebb_request_header_t header = net_ebb_request_header_find_by_index(request, header_index);
    if (header == NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: header [%d]???: set value %.*s no header of index",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            header_index, (int)length, at);
        return;
    }

    if (net_ebb_request_header_set_value(header, at, length) != 0) {
        CPE_INFO(
            service->m_em, "ebb: %s: header [%d]%s: set value %.*s fail",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            header->m_index, header->m_name, (int)length, at);
    }
}

void net_ebb_request_on_body(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on body: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint),
            (int)length, at);
    }
}

void net_ebb_request_on_head_complete(net_ebb_request_t request) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on head complete",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint));
    }

    CPE_INFO(
        service->m_em, "ebb: xxx %s",
        net_ebb_request_dump(net_ebb_service_tmp_buffer(service), request));
}

void net_ebb_request_on_complete(net_ebb_request_t request) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(connection->m_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: on request complete",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), connection->m_endpoint));
    }
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
