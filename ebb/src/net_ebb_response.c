#include <assert.h>
#include "net_endpoint.h"
#include "net_endpoint_stream.h"
#include "net_ebb_response_i.h"

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
    if (request) {
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
    response->m_transfer_encoding = net_ebb_request_transfer_encoding_unknown;
    request->m_response = response;
    
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
    net_endpoint_t endpoint = connection->m_endpoint;

    if (response->m_state != net_ebb_response_state_init) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append code: current state is %s, can`t append code!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    struct net_endpoint_write_stream ws = NET_ENDPOINT_WRITE_STREAM_TO_BUF(endpoint, net_ep_buf_write);
    stream_printf(
        (write_stream_t)&ws, "HTTP/%d.%d %d %s\n"
        , request->m_version_major
        , request->m_version_minor
        , http_code, http_code_msg);

    response->m_state = net_ebb_response_state_head;
    
    return 0;
}

int net_ebb_response_append_head(net_ebb_response_t response, const char * name, const char * value) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = connection->m_endpoint;

    if (response->m_state != net_ebb_response_state_head) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append head: current state is %s, can`t append head!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    struct net_endpoint_write_stream ws = NET_ENDPOINT_WRITE_STREAM_TO_BUF(endpoint, net_ep_buf_write);
    stream_printf((write_stream_t)&ws, "%s: %s\n" , name, value);

    return 0;
}

int net_ebb_response_append_head_line(net_ebb_response_t response, const char * head_line) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = connection->m_endpoint;

    if (response->m_state != net_ebb_response_state_head) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append head: current state is %s, can`t append head!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    struct net_endpoint_write_stream ws = NET_ENDPOINT_WRITE_STREAM_TO_BUF(endpoint, net_ep_buf_write);
    stream_printf((write_stream_t)&ws, "%s\n" , head_line);

    return 0;
}

int net_ebb_response_append_body_identity_begin(net_ebb_response_t response, uint32_t size) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = connection->m_endpoint;

    if (response->m_state != net_ebb_response_state_head) {
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append identity begin: current state is %s, can`t append identity begin!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    response->m_state = net_ebb_response_state_body;
    response->m_identity.m_tota_size = size;
    response->m_identity.m_writed_size = 0;

    struct net_endpoint_write_stream ws = NET_ENDPOINT_WRITE_STREAM_TO_BUF(endpoint, net_ep_buf_write);
    stream_printf((write_stream_t)&ws, "\n");

    return 0;
}

int net_ebb_response_append_body_identity_data(net_ebb_response_t response, void const * data, size_t data_size);

int net_ebb_response_append_body_chunked_begin(net_ebb_response_t response);
int net_ebb_response_append_body_chunked_block(net_ebb_response_t response, void const * data, size_t data_size);

int net_ebb_response_append_complete(net_ebb_response_t response) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = connection->m_endpoint;

    struct net_endpoint_write_stream ws = NET_ENDPOINT_WRITE_STREAM_TO_BUF(endpoint, net_ep_buf_write);
    
    switch(response->m_state) {
    case net_ebb_response_state_head:
    case net_ebb_response_state_done:
        CPE_ERROR(
            service->m_em, "ebb: %s: response: append complete: current state is %s, can`t complete!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    case net_ebb_response_state_init:
        assert(response->m_transfer_encoding == net_ebb_request_transfer_encoding_unknown);
        break;
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
    
    stream_printf((write_stream_t)&ws, "\n");
    
    return 0;
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
