#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_http_svr_request_i.h"
#include "net_http_svr_request_header_i.h"
#include "net_http_svr_mount_point_i.h"
#include "net_http_svr_processor_i.h"
#include "net_http_svr_response_i.h"

net_http_svr_request_t
net_http_svr_request_create(net_endpoint_t base_endpoint) {
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    net_http_svr_request_t request = TAILQ_FIRST(&service->m_free_requests);
    if (request) {
        TAILQ_REMOVE(&service->m_free_requests, request, m_next_for_connection);
    }
    else {
        request = mem_alloc(service->m_alloc, sizeof(struct net_http_svr_request) + service->m_request_sz);
        if (request == NULL) {
            CPE_ERROR(
                service->m_em, "http_svr: %s: request: alloc fail!",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint));
            return NULL;
        }
    }

    request->m_base_endpoint = base_endpoint;
    request->m_processor = NULL;
    request->m_request_id = ++service->m_max_request_id;
    request->m_method = net_http_svr_request_method_unknown;
    request->m_expect_continue = 0;
    request->m_body_read = 0;
    request->m_content_length = 0;
    request->m_version_major = 0;
    request->m_version_minor = 0;
    request->m_number_of_headers = 0;
    request->m_transfer_encoding = net_http_svr_request_transfer_encoding_identity;
    request->m_keep_alive = -1;
    request->m_host = NULL;
    request->m_path = NULL;
    request->m_state = net_http_svr_request_state_reading;
    request->m_response = NULL;
    TAILQ_INIT(&request->m_headers);

    cpe_hash_entry_init(&request->m_hh_for_protocol);
    if (cpe_hash_table_insert_unique(&service->m_requests, request) != 0) {
        CPE_ERROR(
            service->m_em, "http_svr: %s: req %d: id duplicate!", 
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id);
        request->m_base_endpoint = (void*)service;
        TAILQ_INSERT_TAIL(&service->m_free_requests, request, m_next_for_connection);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&connection->m_requests, request, m_next_for_connection);
    
    return request;
}

void net_http_svr_request_free(net_http_svr_request_t request) {
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(request->m_base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(request->m_base_endpoint);

    net_http_svr_request_set_processor(request, NULL);
    assert(request->m_processor == NULL);

    if (request->m_response) {
        net_http_svr_response_free(request->m_response);
        request->m_response = NULL;
    }

    if (request->m_host) {
        mem_free(service->m_alloc, request->m_host);
        request->m_host = NULL;
    }

    if (request->m_path) {
        mem_free(service->m_alloc, request->m_path);
        request->m_path = NULL;
    }

    while(!TAILQ_EMPTY(&request->m_headers)) {
        net_http_svr_request_header_free(TAILQ_FIRST(&request->m_headers));
    }

    cpe_hash_table_remove_by_ins(&service->m_requests, request);

    TAILQ_REMOVE(&connection->m_requests, request, m_next_for_connection);
    
    request->m_base_endpoint = (void*)service;
    TAILQ_INSERT_TAIL(&service->m_free_requests, request, m_next_for_connection);
}

void net_http_svr_request_real_free(net_http_svr_request_t request) {
    net_http_svr_protocol_t service = (void*)request->m_base_endpoint;
    
    TAILQ_REMOVE(&service->m_free_requests, request, m_next_for_connection);
    
    mem_free(service->m_alloc, request);
}

uint32_t net_http_svr_request_id(net_http_svr_request_t request) {
    return request->m_request_id;
}

net_http_svr_request_t
net_http_svr_request_find(net_http_svr_protocol_t service, uint32_t id) {
    struct net_http_svr_request key;
    key.m_request_id = id;
    return cpe_hash_table_find(&service->m_requests, &key);
}

net_http_svr_request_method_t net_http_svr_request_method(net_http_svr_request_t request) {
    return request->m_method;
}

net_http_svr_request_transfer_encoding_t net_http_svr_request_transfer_encoding(net_http_svr_request_t request) {
    return request->m_transfer_encoding;
}

uint8_t net_http_svr_request_expect_continue(net_http_svr_request_t request) {
    return request->m_expect_continue;
}

uint16_t net_http_svr_request_version_major(net_http_svr_request_t request) {
    return request->m_version_major;
}

uint16_t net_http_svr_request_version_minor(net_http_svr_request_t request) {
    return request->m_version_minor;
}

const char * net_http_svr_request_full_path(net_http_svr_request_t request) {
    return request->m_path;
}

const char * net_http_svr_request_relative_path(net_http_svr_request_t request) {
    return request->m_path_to_processor;
}

net_http_svr_request_state_t net_http_svr_request_state(net_http_svr_request_t request) {
    return request->m_state;
}

void net_http_svr_request_schedule_close_connection(net_http_svr_request_t request) {
    net_http_svr_request_set_state(request, net_http_svr_request_state_complete);
    net_http_svr_endpoint_schedule_close(request->m_base_endpoint);
}

const char * net_http_svr_request_find_header_value(net_http_svr_request_t request, const char * name) {
    net_http_svr_request_header_t header = net_http_svr_request_header_find_by_name(request, name);
    return header ? header->m_value : NULL;
}

const char * net_http_svr_request_content_type(net_http_svr_request_t request) {
    return net_http_svr_request_find_header_value(request, "Content-Type");
}

uint8_t net_http_svr_request_content_part_is_text(const char * value) {
    if (strcasecmp(value, "text") == 0) return 1;
    if (strcasecmp(value, "json") == 0) return 1;
    return 0;
}

uint8_t net_http_svr_request_content_is_text(net_http_svr_request_t request) {
    net_http_svr_request_header_t header = net_http_svr_request_header_find_by_name(request, "Content-Type");
    if (header == NULL) return 0;

    char * sep = strchr(header->m_value, '/');
    if (sep == NULL) {
        return net_http_svr_request_content_part_is_text(header->m_value);
    }
    else {
        *sep = 0;
        uint8_t is_text = net_http_svr_request_content_part_is_text(header->m_value);
        *sep = '/';

        if (!is_text) {
            is_text = net_http_svr_request_content_part_is_text(sep + 1);
        }

        return is_text;
    }
}

uint8_t net_http_svr_request_should_keep_alive(net_http_svr_request_t request) {
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

void * net_http_svr_request_data(net_http_svr_request_t request) {
    return request + 1;
}

net_http_svr_request_t net_http_svr_request_from_data(void * data) {
    return ((net_http_svr_request_t)data) - 1;
}

void net_http_svr_request_set_processor(net_http_svr_request_t request, net_http_svr_mount_point_t mp) {
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

void net_http_svr_request_set_state(net_http_svr_request_t request, net_http_svr_request_state_t state) {
    if (request->m_state == state) return;

    if (net_endpoint_protocol_debug(request->m_base_endpoint)) {
        net_endpoint_t base_endpoint = request->m_base_endpoint;
        net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
        net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: state %s ==> %s",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id,
            net_http_svr_request_state_str(request->m_state), 
            net_http_svr_request_state_str(state));
    }

    request->m_state = state;
}

void net_http_svr_request_print_full(write_stream_t ws, net_http_svr_request_t request) {
    stream_printf(
        ws, "%s %d.%d", 
        net_http_svr_request_method_str(request->m_method),
        request->m_version_major, request->m_version_minor);
    
    if (request->m_processor) {
        stream_printf(
            ws, " %s[%s]", 
            request->m_processor->m_name, request->m_path_to_processor);
    }
    
    stream_printf(ws, " (%s)", request->m_path);
    
    net_http_svr_request_header_t header;
    TAILQ_FOREACH(header, &request->m_headers, m_next) {
        stream_printf(ws, "\n    %s: %s", header->m_name, header->m_value);
    }
}

const char * net_http_svr_request_dump_full(mem_buffer_t buffer, net_http_svr_request_t request) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);
    net_http_svr_request_print_full((write_stream_t)&stream, request);
    stream_putc((write_stream_t)&stream, 0);
    return mem_buffer_make_continuous(buffer, 0);
}

void net_http_svr_request_print(write_stream_t ws, net_http_svr_request_t request) {
    stream_printf(
        ws, "%s %d.%d", 
        net_http_svr_request_method_str(request->m_method),
        request->m_version_major, request->m_version_minor);
    
    if (request->m_processor) {
        stream_printf(
            ws, " %s[%s]", 
            request->m_processor->m_name, request->m_path_to_processor);
    }
    
    stream_printf(ws, " (%s)", request->m_path);
}

const char * net_http_svr_request_dump(mem_buffer_t buffer, net_http_svr_request_t request) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);
    net_http_svr_request_print((write_stream_t)&stream, request);
    stream_putc((write_stream_t)&stream, 0);
    return mem_buffer_make_continuous(buffer, 0);
}

void net_http_svr_request_on_path(net_http_svr_request_t request, const char* at, size_t length) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    assert(request->m_path == NULL);
    
    request->m_path = cpe_str_mem_dup_len(service->m_alloc, at, length);
    if (request->m_path == NULL) {
        CPE_ERROR(
            service->m_em, "http_svr: %s: req %d: on path: dup path %.*s fail!", 
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, (int)length, at);
        net_http_svr_endpoint_schedule_close(base_endpoint);
        return;
    }
    
    request->m_path_to_processor = request->m_path;

    net_http_svr_mount_point_t mp = net_http_svr_mount_point_find_by_path(service, &request->m_path_to_processor);
    if (mp == NULL) {
        CPE_ERROR(
            service->m_em, "http_svr: %s: req %d: on path: find processor at  path %s fail!", 
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, request->m_path);
        net_http_svr_endpoint_schedule_close(base_endpoint);
        return;
    }

    net_http_svr_request_set_processor(request, mp);
    
    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: path %s[%s] (full-path=%s)",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, request->m_processor->m_name, request->m_path_to_processor, request->m_path);
    }
}

void net_http_svr_request_on_query_string(net_http_svr_request_t request, const char* at, size_t length) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);
    
    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: query string: %.*s",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, (int)length, at);
    }
}

void net_http_svr_request_on_uri(net_http_svr_request_t request, const char* at, size_t length) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);
    
    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: uri: %.*s",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, (int)length, at);
    }

    const char * sep = cpe_strnstr(at, "://", length);
    if (sep == NULL) {
        if (net_endpoint_protocol_debug(base_endpoint)) {
            CPE_INFO(
                service->m_em, "http_svr: %s: req %d: uri: no protocol sep",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
                request->m_request_id);
        }
    }
    else {
        length -= (sep - at) + 3;
        at = sep + 3;
    }

    const char * host_port_last = cpe_strnchr(at, '/', length);
    if (host_port_last == NULL) host_port_last = cpe_strnchr(at, '?', length);
    if (host_port_last == NULL) host_port_last = at + length;

    request->m_host = cpe_str_mem_dup_range(service->m_alloc, at, host_port_last);
    if (request->m_host == NULL) {
        CPE_ERROR(
            service->m_em, "http_svr: %s: req %d: dup host %.*s fail",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, (int)(host_port_last - at), at);
        net_http_svr_endpoint_schedule_close(base_endpoint);
        return;
    }

    length -= (host_port_last - at);
    at = host_port_last;

    if (request->m_path == NULL) {
        request->m_path = cpe_str_mem_dup_len(service->m_alloc, at, length);
        if (request->m_path == NULL) {
            CPE_ERROR(
                service->m_em, "http_svr: %s: req %d: on uri: dup path %.*s fail!",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
                request->m_request_id, (int)length, at);
            net_http_svr_endpoint_schedule_close(base_endpoint);
            return;
        }

        request->m_path_to_processor = request->m_path;

        net_http_svr_mount_point_t mp = net_http_svr_mount_point_find_by_path(service, &request->m_path_to_processor);
        if (mp == NULL) {
            CPE_ERROR(
                service->m_em, "http_svr: %s: req %d: on uri: find processor at  path %s fail!",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
                request->m_request_id, request->m_path);
            net_http_svr_endpoint_schedule_close(base_endpoint);
            return;
        }

        net_http_svr_request_set_processor(request, mp);

        if (net_endpoint_protocol_debug(base_endpoint)) {
            CPE_INFO(
                service->m_em, "http_svr: %s: req %d: uri: %s[%s] (host=%s, full-path=%s)",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
                request->m_request_id, request->m_processor->m_name, request->m_path_to_processor,
                request->m_host, request->m_path);
        }
    }
}

void net_http_svr_request_on_fragment(net_http_svr_request_t request, const char* at, size_t length) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: on fragment: %.*s",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, (int)length, at);
    }
}

void net_http_svr_request_on_header_field(net_http_svr_request_t request, const char* at, size_t length, int header_index) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    net_http_svr_request_header_t header = net_http_svr_request_header_create(request, (uint16_t)header_index, at, length);
    if (header == NULL) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: header %d: <= %.*s create header fail",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, header_index, (int)length, at);
    }
}

void net_http_svr_request_on_header_value(net_http_svr_request_t request, const char* at, size_t length, int header_index) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    net_http_svr_request_header_t header = net_http_svr_request_header_find_by_index(request, header_index);
    if (header == NULL) {
        CPE_ERROR(
            service->m_em, "http_svr: %s: req %d: header [%d]???: set value %.*s no header of index",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, header_index, (int)length, at);
        net_http_svr_endpoint_schedule_close(base_endpoint);
        return;
    }

    if (net_http_svr_request_header_set_value(header, at, length) != 0) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: header %d: %s: set value %.*s fail",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, header->m_index, header->m_name, (int)length, at);
    }
    
    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: header %d: <= %s: %s",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
            request->m_request_id, header_index, header->m_name, header->m_value);
    }
}

void net_http_svr_request_on_body(net_http_svr_request_t request, const char* at, size_t length) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    if (net_endpoint_protocol_debug(base_endpoint)) {
        if (net_http_svr_request_content_is_text(request)) {
            CPE_INFO(
                service->m_em, "http_svr: %s: req %d: on body: %.*s",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint),
                request->m_request_id, (int)length, at);
        } else {
            char buf[512];
            cpe_str_dup(buf, sizeof(buf), net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint));
            
            CPE_INFO(
                service->m_em, "http_svr: %s: req %d: on body:\n%s",
                buf, request->m_request_id,
                mem_buffer_dump_data(net_http_svr_protocol_tmp_buffer(service), at, length, 0));
        }
    }
}

void net_http_svr_request_on_head_complete(net_http_svr_request_t request) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: on head complete",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint), request->m_request_id);
    }

    if (request->m_state == net_http_svr_request_state_complete) return;
    
    net_http_svr_processor_t processor = request->m_processor;
    if (processor && processor->m_request_on_head_complete) {
        processor->m_request_on_head_complete(processor->m_ctx, request);
    }
}

void net_http_svr_request_on_complete(net_http_svr_request_t request) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
        CPE_INFO(
            service->m_em, "http_svr: %s: req %d: on all complete",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint), request->m_request_id);
    }

    if (request->m_state != net_http_svr_request_state_complete) {
        net_http_svr_processor_t processor = request->m_processor;
        if (processor && processor->m_request_on_complete) {
            processor->m_request_on_complete(processor->m_ctx, request);
        }
    }

    if (request->m_state == net_http_svr_request_state_reading) {
        net_http_svr_request_set_state(request, net_http_svr_request_state_processing);
    }

    if (request->m_state == net_http_svr_request_state_processing) {
        if (request->m_processor == NULL) {
            /* CPE_INFO( */
            /*     service->m_em, "http_svr: %s: req %d: no processor, auto donw", */
            /*     net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint), request->m_request_id); */
        }
    }

    net_http_svr_endpoint_check_remove_done_requests(connection);
}

uint32_t net_http_svr_request_hash(net_http_svr_request_t o) {
    return o->m_request_id;
}

int net_http_svr_request_eq(net_http_svr_request_t l, net_http_svr_request_t r) {
    return l->m_request_id == r->m_request_id ? 1 : 0;
}

const char * net_http_svr_request_method_str(net_http_svr_request_method_t method) {
    switch(method) {
    case net_http_svr_request_method_unknown:
        return "unknown";
    case net_http_svr_request_method_copy:
        return "copy";
    case net_http_svr_request_method_delete:
        return "delete";
    case net_http_svr_request_method_get:
        return "get";
    case net_http_svr_request_method_head:
        return "head";
    case net_http_svr_request_method_lock:
        return "lock";
    case net_http_svr_request_method_mkcol:
        return "mkcol";
    case net_http_svr_request_method_move:
        return "move";
    case net_http_svr_request_method_options:
        return "options";
    case net_http_svr_request_method_post:
        return "post";
    case net_http_svr_request_method_propfind:
        return "propfind";
    case net_http_svr_request_method_proppatch:
        return "proppatch";
    case net_http_svr_request_method_put:
        return "put";
    case net_http_svr_request_method_trace:
        return "trace";
    case net_http_svr_request_method_unlock:
        return "unlock";
    }
}

const char * net_http_svr_request_state_str(net_http_svr_request_state_t state) {
    switch(state) {
    case net_http_svr_request_state_reading:
        return "reading";
    case net_http_svr_request_state_processing:
        return "processing";
    case net_http_svr_request_state_complete:
        return "complete";
    }
}

const char * net_http_svr_request_transfer_encoding_str(net_http_svr_request_transfer_encoding_t transfer_encoding) {
    switch(transfer_encoding) {
    case net_http_svr_request_transfer_encoding_unknown:
        return "unknown";
    case net_http_svr_request_transfer_encoding_identity:
        return "identity";
    case net_http_svr_request_transfer_encoding_chunked:
        return "chunked";
    }
}
