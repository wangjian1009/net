#include "net_endpoint.h"
#include "net_ebb_request_i.h"

static void net_ebb_request_on_path(net_ebb_request_t, const char* at, size_t length);
static void net_ebb_request_on_query_string(net_ebb_request_t, const char* at, size_t length);
static void net_ebb_request_on_uri(net_ebb_request_t, const char* at, size_t length);
static void net_ebb_request_on_fragment(net_ebb_request_t, const char* at, size_t length);

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
    request->on_header_field = NULL;
    request->on_header_value = NULL;
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

uint32_t net_ebb_request_method(net_ebb_request_t request) {
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

static void net_ebb_request_on_path(net_ebb_request_t request, const char* at, size_t length) {
}

static void net_ebb_request_on_query_string(net_ebb_request_t request, const char* at, size_t length) {
}

static void net_ebb_request_on_uri(net_ebb_request_t request, const char* at, size_t length) {
}

static void net_ebb_request_on_fragment(net_ebb_request_t request, const char* at, size_t length) {
}

