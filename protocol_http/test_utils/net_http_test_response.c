#include "test_memory.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/hex_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "net_http_test_response.h"

int net_http_test_response_on_res_begin(void * ctx, net_http_req_t req, uint16_t code, const char * msg) {
    net_http_test_response_t response = ctx;

    response->m_code = code;

    if (response->m_code_msg) {
        mem_free(test_allocrator(), response->m_code_msg);
        response->m_code_msg = NULL;
    }

    if (msg) {
        response->m_code_msg = cpe_str_mem_dup(test_allocrator(), msg);
    }
    
    return 0;
}

int net_http_test_response_on_res_header(void * ctx, net_http_req_t req, const char * name, const char * value) {
    net_http_test_response_t response = ctx;

    if (response->m_head_count < CPE_ARRAY_SIZE(response->m_headers)) {
        response->m_headers[response->m_head_count].m_name = cpe_str_mem_dup(test_allocrator(), name);
        response->m_headers[response->m_head_count].m_value = cpe_str_mem_dup(test_allocrator(), value);
        response->m_head_count++;
    }
    
    return 0;
}

int net_http_test_response_on_res_body(void * ctx, net_http_req_t req, void * data, size_t data_size) {
    net_http_test_response_t response = ctx;
    mem_buffer_append(&response->m_body, data, data_size);
    return 0;
}

void net_http_test_response_on_res_complete(
    void * ctx, net_http_req_t req, net_http_res_result_t result, void * body, uint32_t body_len)
{
    net_http_test_response_t response = ctx;

    response->m_result = result;
    response->m_runing_req = NULL;
}

net_http_test_response_t
net_http_test_response_create(net_http_test_protocol_t protocol, net_http_req_t req) {
    net_http_test_response_t response = mem_alloc(test_allocrator(), sizeof(struct net_http_test_response));

    response->m_protocol = protocol;
    response->m_req_id = net_http_req_id(req);
    response->m_runing_req = req;
    response->m_result = net_http_res_complete;

    response->m_code = 0;
    response->m_code_msg = NULL;
    response->m_head_count = 0;
    
    if (net_http_req_set_reader(
            req,
            response,
            net_http_test_response_on_res_begin,
            net_http_test_response_on_res_header,
            net_http_test_response_on_res_body,
            net_http_test_response_on_res_complete) != 0)
    {
        mem_free(test_allocrator(), response);
        return NULL;
    }

    mem_buffer_init(&response->m_body, test_allocrator());
    
    TAILQ_INSERT_TAIL(&protocol->m_responses, response, m_next);
    return response;
}

const char *
net_http_test_response_find_head_value(net_http_test_response_t response, const char * name) {
    uint16_t i;
    for (i = 0; i < response->m_head_count; i++) {
        if (strcasecmp(response->m_headers[i].m_name, name) == 0) return response->m_headers[i].m_value;
    }
    
    return NULL;
}

void net_http_test_response_free(net_http_test_response_t response) {
    net_http_test_protocol_t protocol = response->m_protocol;

    if (response->m_runing_req) {
        net_http_req_clear_reader(response->m_runing_req);
        response->m_runing_req = NULL;
    }

    if (response->m_code_msg) {
        mem_free(test_allocrator(), response->m_code_msg);
        response->m_code_msg = NULL;
    }

    uint16_t i;
    for(i = 0; i < response->m_head_count; ++i) {
        mem_free(test_allocrator(), response->m_headers[i].m_name);
        mem_free(test_allocrator(), response->m_headers[i].m_value);
    }
    response->m_head_count = 0;
    
    mem_buffer_clear(&response->m_body);
    
    TAILQ_REMOVE(&protocol->m_responses, response, m_next);
    mem_free(test_allocrator(), response);
}

const char * net_http_test_response_body_to_string(net_http_test_response_t response) {
    mem_buffer_append_char(&response->m_body, 0);
    return mem_buffer_make_continuous(&response->m_body, 0);
}

void net_http_test_response_print(write_stream_t ws, net_http_test_response_t response) {
    stream_printf(ws, "%d %s\n", response->m_code, response->m_code_msg);

    uint16_t i;
    for(i = 0; i < response->m_head_count; ++i) {
        stream_printf(ws, "%s=%s\n", response->m_headers[i].m_name, response->m_headers[i].m_value);
    }

    stream_printf(ws, "body.size=%d\n", mem_buffer_size(&response->m_body));

    if (mem_buffer_size(&response->m_body) > 0) {
        const char * content_type = net_http_test_response_find_head_value(response, "Content-Type");
        if (content_type && strstr(content_type, "text") == 0) {
            stream_write(ws, mem_buffer_make_continuous(&response->m_body, 0), mem_buffer_size(&response->m_body));
        }
        else {
            cpe_hex_print_bin_2_hex(
                ws,
                mem_buffer_make_continuous(&response->m_body, 0), mem_buffer_size(&response->m_body));
        }
    }
}

const char * net_http_test_response_dump(mem_buffer_t buffer, net_http_test_response_t response) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);

    net_http_test_response_print((write_stream_t)&stream, response);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}
