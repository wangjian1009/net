#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_http_svr_request_header_i.h"

net_http_svr_request_header_t
net_http_svr_request_header_create(net_http_svr_request_t request, uint16_t index, const char * name, size_t name_len) {
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    net_http_svr_request_header_t header = TAILQ_FIRST(&service->m_free_request_headers);
    if (header) {
        TAILQ_REMOVE(&service->m_free_request_headers, header, m_next);
    }
    else {
        header = mem_alloc(service->m_alloc, sizeof(struct net_http_svr_request_header));
        if (header == NULL) {
            CPE_ERROR(
                service->m_em, "http_svr: %s: header %s: alloc fail!",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint), name);
            return NULL;
        }
    }

    header->m_index = index;
    header->m_request = request;
    header->m_value = NULL;
    
    if (name_len + 1 > sizeof(header->m_buf)) {
        header->m_name = cpe_str_mem_dup_len(service->m_alloc, name, name_len);
        if (header->m_name == NULL) {
            CPE_ERROR(
                service->m_em, "http_svr: %s: header %.*s: dup name fail!",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint), (int)name_len, name);
            header->m_request = (net_http_svr_request_t)service;
            TAILQ_INSERT_TAIL(&service->m_free_request_headers, header, m_next);
            return NULL;
        }
    }
    else {
        memcpy(header->m_buf, name, name_len);
        header->m_buf[name_len] = 0;
        header->m_name = header->m_buf;
    }

    TAILQ_INSERT_TAIL(&request->m_headers, header, m_next);
    
    return header;
}

void net_http_svr_request_header_free(net_http_svr_request_header_t header) {
    net_http_svr_request_t request = header->m_request;
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(request->m_base_endpoint);

    if (header->m_name) {
        if (header->m_name < header->m_buf || header->m_name >= (header->m_buf + sizeof(header->m_buf))) {
            mem_free(service->m_alloc, header->m_name);
        }
        header->m_name = NULL;
    }

    if (header->m_value) {
        if (header->m_value < header->m_buf || header->m_value >= (header->m_buf + sizeof(header->m_buf))) {
            mem_free(service->m_alloc, header->m_value);
        }
        header->m_value = NULL;
    }
    
    TAILQ_REMOVE(&request->m_headers, header, m_next);
    
    header->m_request = (net_http_svr_request_t)service;
    TAILQ_INSERT_TAIL(&service->m_free_request_headers, header, m_next);
}

void net_http_svr_request_header_real_free(net_http_svr_request_header_t header) {
    net_http_svr_protocol_t service = (net_http_svr_protocol_t)header->m_request;
    TAILQ_REMOVE(&service->m_free_request_headers, header, m_next);
    mem_free(service->m_alloc, header);
}

int net_http_svr_request_header_set_value(net_http_svr_request_header_t header, const char * value, size_t value_len) {
    net_http_svr_request_t request = header->m_request;
    net_endpoint_t base_endpoint = request->m_base_endpoint;
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    if (header->m_value) {
        if (header->m_value < header->m_buf || header->m_value >= (header->m_buf + sizeof(header->m_buf))) {
            mem_free(service->m_alloc, header->m_value);
        }
        header->m_value = NULL;
    }

    char * buf = header->m_buf;
    size_t buf_len = sizeof(header->m_buf);
    
    if (header->m_name == header->m_buf) {
        size_t name_len = strlen(header->m_name) + 1;
        assert(name_len <= buf_len);
        buf_len -= name_len;
        buf += name_len;
    }

    if (value_len + 1 > buf_len) {
        header->m_value = cpe_str_mem_dup_len(service->m_alloc, value, value_len);
        if (header->m_value == NULL) {
            CPE_ERROR(
                service->m_em, "http_svr: %s: header %s: dup value %s fail!",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint), header->m_name, value);
            return -1;
        }
    }
    else {
        memcpy(buf, value, value_len);
        buf[value_len] = 0;
        header->m_value = buf;
    }
    
    return 0;
}

net_http_svr_request_header_t
net_http_svr_request_header_find_by_index(net_http_svr_request_t request, uint16_t index) {
    net_http_svr_request_header_t header;
    
    TAILQ_FOREACH_REVERSE(header, &request->m_headers, net_http_svr_request_header_list, m_next) {
        if (header->m_index == index) return header;
    }
    
    return NULL;
}
