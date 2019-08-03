#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "net_endpoint.h"
#include "net_ebb_response_i.h"

static int net_ebb_response_append_head_last(net_ebb_response_t response);
static int net_ebb_response_append(net_ebb_response_t response, void const * data, uint32_t data_size);

net_ebb_response_t
net_ebb_response_create(net_ebb_request_t request) {
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);

    if (request->m_response != NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: request already have response!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)));
        net_ebb_request_schedule_close_connection(request);
        return NULL;
    }
    
    net_ebb_response_t response = TAILQ_FIRST(&service->m_free_responses);
    if (response) {
        TAILQ_REMOVE(&service->m_free_responses, response, m_next);
    }
    else {
        response = mem_alloc(service->m_alloc, sizeof(struct net_ebb_response));
        if (response == NULL) {
            CPE_ERROR(
                service->m_em, "ebb: %s: response: alloc fail!",
                net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)));
            net_ebb_request_schedule_close_connection(request);
            return NULL;
        }
    }

    response->m_request = request;
    response->m_state = net_ebb_response_state_init;
    response->m_header_count = 0;
    response->m_head_connection_setted = 0;
    response->m_transfer_encoding = net_ebb_request_transfer_encoding_unknown;
    request->m_response = response;

    /* CPE_ERROR( */
    /*     service->m_em, "ebb: %s: response: created!", */
    /*     net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection))); */
    
    return response;
}

net_ebb_response_t net_ebb_response_get(net_ebb_request_t request) {
    return request->m_response;
}

void net_ebb_response_free(net_ebb_response_t response) {
    net_ebb_request_t request = response->m_request;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    response->m_request = (net_ebb_request_t)service;
    TAILQ_INSERT_TAIL(&service->m_free_responses, response, m_next);
}

void net_ebb_response_real_free(net_ebb_response_t response) {
    net_ebb_service_t service = (net_ebb_service_t)response->m_request;
    
    TAILQ_REMOVE(&service->m_free_responses, response, m_next);
    mem_free(service->m_alloc, response);
}

int net_ebb_response_append_code(net_ebb_response_t response, int http_code, const char * http_code_msg) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);
    
    if (response->m_state != net_ebb_response_state_init) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append code: current state is %s, can`t append code!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    mem_buffer_t buffer = &service->m_data_buffer;
    mem_buffer_clear_data(buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    
    stream_printf(
        (write_stream_t)&ws, "HTTP/%d.%d %d %s"
        , request->m_version_major
        , request->m_version_minor
        , http_code, http_code_msg);

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: header %d: => %.*s", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, response->m_header_count, (int)mem_buffer_size(buffer), mem_buffer_make_continuous(buffer, 0));
    }

    mem_buffer_append_char(buffer, '\n');
    
    response->m_header_count++;
    response->m_state = net_ebb_response_state_head;

    return net_ebb_response_append(response, mem_buffer_make_continuous(buffer, 0), mem_buffer_size(buffer));
}

int net_ebb_response_append_head(net_ebb_response_t response, const char * name, const char * value) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);
    
    if (response->m_state != net_ebb_response_state_head) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append head: %s: %s: current state is %s, can`t append head!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            name, value, net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    mem_buffer_t buffer = &service->m_data_buffer;
    mem_buffer_clear_data(buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    stream_printf((write_stream_t)&ws, "%s: %s" , name, value);

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: header %d: => %.*s", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, response->m_header_count, (int)mem_buffer_size(buffer), mem_buffer_make_continuous(buffer, 0));
    }

    mem_buffer_append_char(buffer, '\n');
    
    response->m_header_count++;
    
    return net_ebb_response_append(response, mem_buffer_make_continuous(buffer, 0), mem_buffer_size(buffer));
}

int net_ebb_response_append_head_line(net_ebb_response_t response, const char * head_line) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);
    
    if (response->m_state != net_ebb_response_state_head) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append head: %s: current state is %s, can`t append head!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            head_line, net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    mem_buffer_t buffer = &service->m_data_buffer;
    mem_buffer_clear_data(buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    stream_printf((write_stream_t)&ws, "%s" , head_line);

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: header %d: => %.*s", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, response->m_header_count, (int)mem_buffer_size(buffer), mem_buffer_make_continuous(buffer, 0));
    }

    mem_buffer_append_char(buffer, '\n');
    
    response->m_header_count++;
    
    return net_ebb_response_append(response, mem_buffer_make_continuous(buffer, 0), mem_buffer_size(buffer));
}

int net_ebb_response_append_body_identity_begin(net_ebb_response_t response, uint32_t size) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);
    
    if (response->m_state != net_ebb_response_state_head) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append identity begin: current state is %s, can`t append identity begin!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    char size_buf[64];
    snprintf(size_buf, sizeof(size_buf), "%d", size);
    if (net_ebb_response_append_head(response, "Content-Length",  size_buf) != 0) return -1;
    if (net_ebb_response_append_head_last(response) != 0) return -1;

    response->m_state = net_ebb_response_state_body;
    response->m_transfer_encoding = net_ebb_request_transfer_encoding_identity;
    response->m_identity.m_tota_size = size;
    response->m_identity.m_writed_size = 0;

    return 0;
}

int net_ebb_response_append_body_identity_data(net_ebb_response_t response, void const * data, size_t data_size) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);
    
    return 0;
}

int net_ebb_response_append_body_identity_data_from_stream(net_ebb_response_t response, read_stream_t rs) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;

    if (response->m_state != net_ebb_response_state_body) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append body: current state is %s, can`t body!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }
    
    net_ebb_connection_timeout_reset(connection);

    return 0;
}

int net_ebb_response_append_body_chunked_begin(net_ebb_response_t response) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;

    net_ebb_connection_timeout_reset(connection);

    return 0;
}

int net_ebb_response_append_body_chunked_block(net_ebb_response_t response, void const * data, size_t data_size) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);
    
    return 0;
}

int net_ebb_response_append_complete(net_ebb_response_t response) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);
    
    switch(response->m_state) {
    case net_ebb_response_state_init:
    case net_ebb_response_state_done:
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append complete: current state is %s, can`t complete!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    case net_ebb_response_state_head:
        assert(response->m_transfer_encoding == net_ebb_request_transfer_encoding_unknown);
        if (net_ebb_response_append_body_identity_begin(response, 0) != 0) return -1;
        /*goto next block*/
    case net_ebb_response_state_body:
        switch (response->m_transfer_encoding) {
        case net_ebb_request_transfer_encoding_unknown:
            assert(0);
            break;
        case net_ebb_request_transfer_encoding_identity:
            break;
        case net_ebb_request_transfer_encoding_chunked:
            break;
        }
        break;
    }

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: => \\n\\n", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)), request->m_request_id);
    }
    
    response->m_state = net_ebb_response_state_done;
    net_ebb_request_set_state(request, net_ebb_request_state_complete);
    
    return net_ebb_response_append(response, "\n\n", 2);
}

/*utils*/
int net_ebb_response_send(net_ebb_request_t request, int http_code, const char * http_code_msg) {
    net_ebb_response_t response = net_ebb_response_create(request);
    if (response == NULL) return -1;

    if (net_ebb_response_append_code(response, http_code, http_code_msg) != 0) return -1;
    if (net_ebb_response_append_complete(response) != 0) return -1;
    
    return 0;
}

const char * net_ebb_response_state_str(net_ebb_response_state_t state) {
    switch(state) {
    case net_ebb_response_state_init:
        return "init";
    case net_ebb_response_state_head:
        return "head";
    case net_ebb_response_state_body:
        return "body";
    case net_ebb_response_state_done:
        return "done";
    }
}

static int net_ebb_response_append_head_last(net_ebb_response_t response) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);
    
    if (request->m_version_major == 1 && request->m_version_minor == 1) {
        if (!response->m_head_connection_setted) {
            if (net_ebb_response_append_head(
                    response,
                    "Connection",
                    net_ebb_request_should_keep_alive(response->m_request) ? "Keep-Alive" : "Close")
                != 0) return -1;
        }
    }

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: header %d: => ", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, response->m_header_count);
    }
    
    const char * head_end = "\n";
    return net_ebb_response_append(response, head_end, strlen(head_end));
}

static int net_ebb_response_append(net_ebb_response_t response, void const * data, uint32_t data_size) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);
    
    if (net_endpoint_buf_append(endpoint, net_ep_buf_write, data, data_size) != 0) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: write response fail!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }
    
    return 0;
}
