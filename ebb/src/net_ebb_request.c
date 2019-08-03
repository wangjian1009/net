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
    request->m_request_id = ++service->m_max_request_id;
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
    request->m_state = net_ebb_request_state_reading;
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

uint32_t net_ebb_request_id(net_ebb_request_t request) {
    return request->m_request_id;
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

net_ebb_request_state_t net_ebb_request_state(net_ebb_request_t request) {
    return request->m_state;
}

void net_ebb_request_schedule_close_connection(net_ebb_request_t request) {
    net_ebb_request_set_state(request, net_ebb_request_state_complete);
    net_ebb_connection_schedule_close(request->m_connection);
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

void * net_ebb_request_data(net_ebb_request_t request) {
    return request + 1;
}

net_ebb_request_t net_ebb_request_from_data(void * data) {
    return ((net_ebb_request_t)data) - 1;
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

void net_ebb_request_set_state(net_ebb_request_t request, net_ebb_request_state_t state) {
    if (request->m_state == state) return;

    if (net_endpoint_protocol_debug(net_endpoint_from_data(request->m_connection))) {
        net_ebb_connection_t connection = request->m_connection;
        net_ebb_service_t service = net_ebb_connection_service(connection);

        CPE_INFO(
            service->m_em, "ebb: %s: req %d: state %s ==> %s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id,
            net_ebb_request_state_str(request->m_state), 
            net_ebb_request_state_str(state));
    }

    request->m_state = state;
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
            service->m_em, "ebb: %s: req %d: on path: dup path %.*s fail!", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, (int)length, at);
        net_ebb_connection_schedule_close(connection);
        return;
    }
    
    request->m_path_to_processor = request->m_path;

    net_ebb_mount_point_t mp = net_ebb_mount_point_find_by_path(service, &request->m_path_to_processor);
    if (mp == NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: on path: find processor at  path %s fail!", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, request->m_path);
        net_ebb_connection_schedule_close(connection);
        return;
    }

    net_ebb_request_set_processor(request, mp);
    
    if (net_endpoint_protocol_debug(net_endpoint_from_data(connection))) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: path %s[%s] (full-path=%s)",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, request->m_processor->m_name, request->m_path_to_processor, request->m_path);
    }
}

void net_ebb_request_on_query_string(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    if (net_endpoint_protocol_debug(net_endpoint_from_data(connection))) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: query string: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, (int)length, at);
    }
}

void net_ebb_request_on_uri(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    if (net_endpoint_protocol_debug(net_endpoint_from_data(connection)) >= 2) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: uri: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, (int)length, at);
    }
}

void net_ebb_request_on_fragment(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(net_endpoint_from_data(connection)) >= 2) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: on fragment: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, (int)length, at);
    }
}

void net_ebb_request_on_header_field(net_ebb_request_t request, const char* at, size_t length, int header_index) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    net_ebb_request_header_t header = net_ebb_request_header_create(request, (uint16_t)header_index, at, length);
    if (header == NULL) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: header %d: <= %.*s create header fail",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, header_index, (int)length, at);
    }
}

void net_ebb_request_on_header_value(net_ebb_request_t request, const char* at, size_t length, int header_index) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    net_ebb_request_header_t header = net_ebb_request_header_find_by_index(request, header_index);
    if (header == NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: header [%d]???: set value %.*s no header of index",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, header_index, (int)length, at);
        return;
    }

    if (net_ebb_request_header_set_value(header, at, length) != 0) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: header %d: %s: set value %.*s fail",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, header->m_index, header->m_name, (int)length, at);
    }
    
    if (net_endpoint_protocol_debug(net_endpoint_from_data(connection))) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: header %d: <= %s: %s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, header_index, header->m_name, header->m_value);
    }
}

void net_ebb_request_on_body(net_ebb_request_t request, const char* at, size_t length) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(net_endpoint_from_data(connection))) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: on body: %.*s",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, (int)length, at);
    }
}

void net_ebb_request_on_head_complete(net_ebb_request_t request) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(net_endpoint_from_data(connection)) >= 2) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: on head complete",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)), request->m_request_id);
    }

    if (request->m_state == net_ebb_request_state_complete) return;
    
    net_ebb_processor_t processor = request->m_processor;
    if (processor && processor->m_request_on_head_complete) {
        processor->m_request_on_head_complete(processor->m_ctx, request);
    }
}

void net_ebb_request_on_complete(net_ebb_request_t request) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);

    if (net_endpoint_protocol_debug(net_endpoint_from_data(connection)) >= 2) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: on all complete",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)), request->m_request_id);
    }
    
    if (request->m_state != net_ebb_request_state_complete) {
        net_ebb_processor_t processor = request->m_processor;
        if (processor && processor->m_request_on_complete) {
            processor->m_request_on_complete(processor->m_ctx, request);
        }
    }

    if (request->m_state == net_ebb_request_state_reading) {
        net_ebb_request_set_state(request, net_ebb_request_state_processing);
    }
    
    net_ebb_connection_check_remove_done_requests(connection);
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

const char * net_ebb_request_state_str(net_ebb_request_state_t state) {
    switch(state) {
    case net_ebb_request_state_reading:
        return "reading";
    case net_ebb_request_state_processing:
        return "processing";
    case net_ebb_request_state_complete:
        return "complete";
    }
}

const char * net_ebb_request_transfer_encoding_str(net_ebb_request_transfer_encoding_t transfer_encoding) {
    switch(transfer_encoding) {
    case net_ebb_request_transfer_encoding_unknown:
        return "unknown";
    case net_ebb_request_transfer_encoding_identity:
        return "identity";
    case net_ebb_request_transfer_encoding_chunked:
        return "chunked";
    }
}
