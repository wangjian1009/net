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
            service->m_em, "ebb: %s: req %d: request already have response!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id);
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
                service->m_em, "ebb: %s: req %d: alloc fail!",
                net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
                request->m_request_id);
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
            service->m_em, "ebb: %s: req %d: append code: current state is %s, can`t append code!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_response_state_str(response->m_state));
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

    stream_printf((write_stream_t)&ws, "\r\n");
    
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
            service->m_em, "ebb: %s: req %d: append head: %s: %s: current state is %s, can`t append head!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, name, value, net_ebb_response_state_str(response->m_state));
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

    stream_printf((write_stream_t)&ws, "\r\n");
    
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
            service->m_em, "ebb: %s: req %d: append head: %s: current state is %s, can`t append head!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, head_line, net_ebb_response_state_str(response->m_state));
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

    stream_printf((write_stream_t)&ws, "\r\n");
    
    response->m_header_count++;
    
    return net_ebb_response_append(response, mem_buffer_make_continuous(buffer, 0), mem_buffer_size(buffer));
}

int net_ebb_response_append_head_minetype_by_postfix(net_ebb_response_t response, const char * postfix, uint8_t * is_added) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    
    if (postfix == NULL) {
        if (is_added) *is_added = 0;
        return 0;
    }

    const char * minetype = net_ebb_service_mine_from_postfix(service, postfix);
    if (minetype == NULL) {
        if (is_added) *is_added = 0;
        return 0;
    }
    
    if (is_added) *is_added = 1;
    return net_ebb_response_append_head(response, "Content-Type", minetype);
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
            service->m_em, "ebb: %s: req %d: append identity begin: current state is %s, can`t append identity begin!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_response_state_str(response->m_state));
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

int net_ebb_response_append_body_identity_data(net_ebb_response_t response, void const * data, uint32_t data_size) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);

    if (response->m_state != net_ebb_response_state_body) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append body: current state is %s, can`t body!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    if (response->m_transfer_encoding != net_ebb_request_transfer_encoding_identity) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append body: current encoding is %s, can`t append identity!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_request_transfer_encoding_str(response->m_transfer_encoding));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    if ((response->m_identity.m_writed_size + data_size) > response->m_identity.m_tota_size) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append body: total-size=%d, curent-size=%d, append %d data overflow!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, response->m_identity.m_tota_size, response->m_identity.m_writed_size, data_size);
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: body: => %d data", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, data_size);
    }
    
    if (net_endpoint_buf_append(endpoint, net_ep_buf_write, data, data_size) != 0) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append body: write %d data fail!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, data_size);
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    response->m_identity.m_writed_size += data_size;
    
    return 0;
}

int net_ebb_response_append_body_identity_from_stream(net_ebb_response_t response, read_stream_t rs) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;

    net_ebb_connection_timeout_reset(connection);
    
    if (response->m_state != net_ebb_response_state_body) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append body: current state is %s, can`t body!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    if (response->m_transfer_encoding != net_ebb_request_transfer_encoding_identity) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append body: current encoding is %s, can`t append identity!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_request_transfer_encoding_str(response->m_transfer_encoding));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    assert(response->m_identity.m_tota_size >= response->m_identity.m_writed_size);
    uint32_t left_sz = response->m_identity.m_tota_size - response->m_identity.m_writed_size;
    
    while(left_sz > 0 && net_endpoint_state(endpoint) == net_endpoint_state_established) {
        uint32_t buf_size = 0;
        void* buf = net_endpoint_buf_alloc(endpoint, &buf_size);
        if (buf == NULL) {
            CPE_ERROR(
                service->m_em, "ebb: %s: req %d: append body: alloc ep buf fail!",
                net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
                request->m_request_id);
            net_ebb_request_schedule_close_connection(request);
            return -1;
        }

        if (buf_size > left_sz) buf_size = left_sz;
        
        int read_sz = stream_read(rs, buf, buf_size);
        if (read_sz < 0) {
            CPE_ERROR(
                service->m_em, "ebb: %s: req %d: append body: read stream fail!",
                net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
                request->m_request_id);
            net_endpoint_buf_release(endpoint);
            net_ebb_request_schedule_close_connection(request);
            return -1;
        }
        else if (read_sz == 0) {
            net_endpoint_buf_release(endpoint);
            return 0;
        }
        else {
            if (net_endpoint_buf_supply(endpoint, net_ep_buf_write, (uint32_t)read_sz) != 0) {
                CPE_ERROR(
                    service->m_em, "ebb: %s: req %d: append body: supply ep buf fail fail!",
                    net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
                    request->m_request_id);
                net_ebb_request_schedule_close_connection(request);
                return -1;
            }

            if (net_endpoint_protocol_debug(endpoint)) {
                CPE_INFO(
                    service->m_em, "ebb: %s: req %d: body: => %d data",
                    net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
                    request->m_request_id, (uint32_t)read_sz);
            }
            
            assert(left_sz >= (uint32_t)read_sz);
            response->m_identity.m_writed_size += (uint32_t)read_sz;
            left_sz -= (uint32_t)read_sz;
            if (left_sz == 0) return 0;
        }
    }

    return -1;
}

int net_ebb_response_append_body_chunked_begin(net_ebb_response_t response) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;

    net_ebb_connection_timeout_reset(connection);

    if (response->m_state != net_ebb_response_state_head) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append identity begin: current state is %s, can`t append identity begin!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    if (net_ebb_response_append_head_line(response, "Transfer-Encoding: chunked") != 0) return -1;
    if (net_ebb_response_append_head_last(response) != 0) return -1;

    response->m_state = net_ebb_response_state_body;
    response->m_transfer_encoding = net_ebb_request_transfer_encoding_chunked;
    response->m_chunked.m_trunk_count = 0;

    return 0;
}

int net_ebb_response_append_body_chunked_block(net_ebb_response_t response, void const * data, uint32_t data_size) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);

    if (request->m_state == net_ebb_request_state_complete) return -1;
    
    net_ebb_connection_timeout_reset(connection);

    if (response->m_state != net_ebb_response_state_body) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append body: current state is %s, can`t body!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_response_state_str(response->m_state));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    if (response->m_transfer_encoding != net_ebb_request_transfer_encoding_chunked) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: append body: current encoding is %s, can`t append chunked!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_request_transfer_encoding_str(response->m_transfer_encoding));
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: req %d: trunked %d: => %d data", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, response->m_chunked.m_trunk_count, data_size);
    }

    char size_buf[64];
    snprintf(size_buf, sizeof(size_buf), "%d\r\n", data_size);
    if (net_endpoint_buf_append(endpoint, net_ep_buf_write, data, data_size) != 0
        || net_endpoint_buf_append(endpoint, net_ep_buf_write, data, data_size) != 0
        || net_endpoint_buf_append(endpoint, net_ep_buf_write, "\r\n", 2) != 0)
    {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: trunked %d: qppend trunked data!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, response->m_chunked.m_trunk_count);
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }

    response->m_chunked.m_trunk_count++;
    
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
            service->m_em, "ebb: %s: req %d: append complete: current state is %s, can`t complete!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, net_ebb_response_state_str(response->m_state));
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
            if (response->m_identity.m_tota_size != response->m_identity.m_writed_size) {
                CPE_ERROR(
                    service->m_em, "ebb: %s: req %d: append complete: identity body expect %d, but send %d size!",
                    net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
                    request->m_request_id, response->m_identity.m_tota_size, response->m_identity.m_writed_size);
                net_ebb_request_schedule_close_connection(request);
                return -1;
            }
            break;
        case net_ebb_request_transfer_encoding_chunked:
            if (net_endpoint_protocol_debug(endpoint)) {
                CPE_INFO(
                    service->m_em, "ebb: %s: req %d: trunked %d: => 0 data",
                    net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
                    request->m_request_id, response->m_chunked.m_trunk_count);
            }

            if (net_endpoint_buf_append(endpoint, net_ep_buf_write, "0\r\n\r\n", 5) != 0) {
                CPE_ERROR(
                    service->m_em, "ebb: %s: req %d: append complete: trunked append last trunk fail!",
                    net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
                    request->m_request_id);
                net_ebb_request_schedule_close_connection(request);
                return -1;
            }
            break;
        }
        break;
    }

    response->m_state = net_ebb_response_state_done;
    net_ebb_request_set_state(request, net_ebb_request_state_complete);

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
    
    if (net_endpoint_buf_append(endpoint, net_ep_buf_write, "\r\n", 2) != 0) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: head %d: write head end fail!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id, response->m_header_count);
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }
    
    return 0;
}

static int net_ebb_response_append(net_ebb_response_t response, void const * data, uint32_t data_size) {
    net_ebb_request_t request = response->m_request;
    net_ebb_connection_t connection = request->m_connection;
    net_ebb_service_t service = net_ebb_connection_service(connection);
    net_endpoint_t endpoint = net_endpoint_from_data(connection);
    
    if (net_endpoint_buf_append(endpoint, net_ep_buf_write, data, data_size) != 0) {
        CPE_ERROR(
            service->m_em, "ebb: %s: req %d: write response fail!",
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), net_endpoint_from_data(connection)),
            request->m_request_id);
        net_ebb_request_schedule_close_connection(request);
        return -1;
    }
    
    return 0;
}
