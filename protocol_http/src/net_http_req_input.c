#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_http_req_i.h"

static int net_http_req_input_read_head(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req, net_endpoint_t endpoint, char * data);

static int net_http_req_input_body_consume_body_part(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint, uint16_t buf_sz);

static int net_http_req_input_body_encoding_none(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint);

static int net_http_req_input_body_encoding_trunked(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint);

int net_http_req_set_reader(
    net_http_req_t req,
    void * ctx,
    net_http_req_on_res_begin_fun_t on_begin,
    net_http_req_on_res_head_fun_t on_head,
    net_http_req_on_res_body_fun_t on_body,
    net_http_req_on_res_complete_fun_t on_complete)
{
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(req->m_http_ep);
    
    if (req->m_res_state != net_http_res_state_reading_head) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: res-state=%s, can`t set reader!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), req->m_http_ep->m_endpoint),
            req->m_id,
            net_http_res_state_str(req->m_res_state));
        return -1;
    }

    req->m_res_ctx = ctx;
    req->m_res_on_begin = on_begin;
    req->m_res_on_head = on_head;
    req->m_res_on_body = on_body;
    req->m_res_on_complete = on_complete;
    
    return 0;
}

uint16_t net_http_req_res_code(net_http_req_t req) {
    return req->m_res_code;
}

const char * net_http_req_res_message(net_http_req_t req) {
    return req->m_res_message;
}

uint32_t net_http_req_res_length(net_http_req_t req) {
    return req->m_res_trans_encoding == net_http_trans_encoding_none ? req->m_res_content.m_length : 0;
}

int net_http_endpoint_req_input(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req) {
    net_endpoint_t endpoint = http_ep->m_endpoint;
    void * buf;
    uint32_t buf_size;

    if (req->m_res_state == net_http_res_state_reading_head) {
        if (net_endpoint_buf_by_str(endpoint, net_ep_buf_http_in, "\r\n\r\n", &buf, &buf_size)) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: response: search sep fail",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                req->m_id);
            return -1;
        }

        if (buf == NULL) {
            if(net_endpoint_buf_size(endpoint, net_ep_buf_http_in) > 8192) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: handshake response: Too big response head!, size=%d",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    net_endpoint_buf_size(endpoint, net_ep_buf_http_in));
                return -1;
            }
            else {
                return 0;
            }
        }

        if (net_http_req_input_read_head(http_protocol, http_ep, req, endpoint, buf) != 0) return -1;

        net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, buf_size);
        
        req->m_res_state = net_http_res_state_reading_body;
    }

    if (req->m_res_state == net_http_res_state_reading_body) {
        switch(req->m_res_trans_encoding) {
        case net_http_trans_encoding_none:
            if (net_http_req_input_body_encoding_none(http_protocol, http_ep, req, endpoint) != 0) return -1;
            break;
        case net_http_trans_encoding_trunked:
            if (net_http_req_input_body_encoding_trunked(http_protocol, http_ep, req, endpoint) != 0) return -1;
            break;
        }
    }
    
    return 0;
}

const char * net_http_res_state_str(net_http_res_state_t res_state) {
    switch(res_state) {
    case net_http_res_state_reading_head:
        return "http-res-reading-head";
    case net_http_res_state_reading_body:
        return "http-res-reading-body";
    case net_http_res_state_completed:
        return "http-res-completed";
    }
}

static int net_http_req_input_read_head_line(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint, char * line, uint32_t line_num)
{
    if (line_num == 0) {
        char * sep1 = strchr(line, ' ');
        char * sep2 = sep1 ? strchr(sep1 + 1, ' ') : NULL;
        if (sep1 == NULL || sep2 == NULL) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: parse head method: first line %s format error",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                req->m_id,
                line);
            return -1;
        }

        char * version = line;
        *sep1 = 0;

        char * code = sep1 + 1;
        *sep2 = 0;

        char * msg = sep2 + 1;

        req->m_res_code = (uint16_t)atoi(code);
        cpe_str_dup(req->m_res_message, sizeof(req->m_res_message), msg);

        if (!req->m_res_ignore && req->m_res_on_begin) {
            switch(req->m_res_on_begin(req->m_res_ctx, req, req->m_res_code, msg)) {
            case net_http_res_op_success:
                break;
            case net_http_res_op_ignore:
                req->m_res_ignore = 1;
                break;
            case net_http_res_op_error_and_reconnect:
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: req %d: <== process res begin fail, version=%s, code=%d, msg=%s",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id,
                    version, req->m_res_code, msg);
                return -1;
            }
        }
        
        *sep1 = ' ';
        *sep2 = ' ';
    }
    else {
        char * sep = strchr(line, ':');
        if (sep == NULL) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: <== head line format error\n%s",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                req->m_id,
                line);
            return -1;
        }

        char * name = line;
        char * value = cpe_str_trim_head(sep + 1);

        sep = cpe_str_trim_tail(sep, line);

        *sep = 0;

        if (strcasecmp(name, "Content-Length") == 0) {
            req->m_res_trans_encoding = net_http_trans_encoding_none;
            char * endptr = NULL;
            req->m_res_content.m_length = (uint32_t)strtol(value, &endptr, 10);
            if (endptr == NULL || *cpe_str_trim_head(endptr) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: req %d: <== Content-Length %s format error",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id,
                    value);
                return -1;
            }
        }
        else if (strcasecmp(name, "Connection") == 0) {
            if (strcasecmp(value, "keep-alive") == 0) {
                http_ep->m_connection_type = net_http_connection_type_keep_alive;
            }
            else if (strcasecmp(value, "close") == 0) {
                http_ep->m_connection_type = net_http_connection_type_close;
            }
            else if (strcasecmp(value, "upgrade") == 0) {
                http_ep->m_connection_type = net_http_connection_type_upgrade;
            }
        }
        else if (strcasecmp(name, "Transfer-Encoding") == 0) {
            if (strcasecmp(value, "chunked") == 0) {
                req->m_res_trans_encoding = net_http_trans_encoding_trunked;
                req->m_res_trunked.m_state = net_http_trunked_length;
                req->m_res_trunked.m_count = 0;
                req->m_res_trunked.m_length = 0;
            }
            else {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: req %d: <== Transfer-Encoding %s unknown",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id, value);
            }
        }
        else if (strcasecmp(name, "X-Request-Id") == 0) {
        }
        
        if (!req->m_res_ignore && req->m_res_on_head) {
            switch(req->m_res_on_head(req->m_res_ctx, req, name, value)) {
            case net_http_res_op_success:
                break;
            case net_http_res_op_ignore:
                req->m_res_ignore = 1;
                break;
            case net_http_res_op_error_and_reconnect:
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: req %d: <== process head fail, %s: %s",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id,
                    name, value);
                return -1;
            }
        }
        
        *sep = ':';
    }
    
    return 0;
}

static int net_http_req_input_read_head(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint, char * data)
{
    if (net_endpoint_protocol_debug(endpoint) >= 2) {
        CPE_INFO(
            http_protocol->m_em, "http: %s: req %d: <== head\n%s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
            req->m_id,
            data);
    }

    char * line = data;
    char * sep = strstr(line, "\r\n");
    uint32_t line_num = 0;
    for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        if (net_http_req_input_read_head_line(http_protocol, http_ep, req, endpoint, line, line_num) != 0) return -1;
        *sep = '\r';
        line_num++;
    }

    if (line[0]) {
        if (net_http_req_input_read_head_line(http_protocol, http_ep, req, endpoint, line, line_num) != 0) return -1;
    }
    
    return 0;
}

static int net_http_req_input_body_set_complete(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req, net_endpoint_t endpoint, net_http_res_result_t result)
{
    req->m_res_state = net_http_res_state_completed;
    
    if (!req->m_res_ignore && req->m_res_on_complete) {
        switch(req->m_res_on_complete(req->m_res_ctx, req, result)) {
        case net_http_res_op_success:
            break;
        case net_http_res_op_ignore:
            req->m_res_ignore = 1;
            break;
        case net_http_res_op_error_and_reconnect:
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: <== on complete fail",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                req->m_id);
            return -1;
        }
    }

    net_endpoint_buf_clear(endpoint, net_ep_buf_http_body);
    return 0;
}

static int net_http_req_input_body_consume_body_part(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint, uint16_t buf_sz)
{
    if (req->m_res_ignore) {
        net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, buf_sz);
        return 0;
    }

    if (req->m_res_on_body) {
        void * buf = NULL;
        if (net_endpoint_buf_peak_with_size(endpoint, net_ep_buf_http_in, buf_sz, &buf) != 0) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: <== peak body data fail, size=%d",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                req->m_id, buf_sz);
            return -1;
        }

        switch(req->m_res_on_body(req->m_res_ctx, req, buf, buf_sz)) {
        case net_http_res_op_success:
            break;
        case net_http_res_op_ignore:
            req->m_res_ignore = 1;
            break;
        case net_http_res_op_error_and_reconnect:
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: <== process body fail, size=%d",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                req->m_id, buf_sz);
            return -1;
        }

        net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, buf_sz);
    }
    else {
        if (net_endpoint_buf_append_from_self(endpoint, net_ep_buf_http_body, net_ep_buf_http_in, buf_sz) != 0) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: <== move body to tmp buf fail, size=%d",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                req->m_id, buf_sz);
            return -1;
        }
    }

    return 0;
}

static int net_http_req_input_body_encoding_none(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint)
{
    if (http_ep->m_connection_type != net_http_connection_type_close
        && req->m_res_content.m_length == 0)
    {
        if (net_http_req_input_body_set_complete(http_protocol, http_ep, req, endpoint, net_http_res_complete) != 0) return -1;
        return 0;
    }
    
    while(req->m_res_state != net_http_res_state_completed) {
        uint32_t buf_sz = net_endpoint_buf_size(endpoint, net_ep_buf_http_in);
        if (buf_sz == 0) break;
        
        if (req->m_res_content.m_length == 0) {
            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: req %d: <== body %d data",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id, buf_sz);
            }

            if (net_http_req_input_body_consume_body_part(http_protocol, http_ep, req, endpoint, buf_sz) != 0) return -1;
        }
        else {
            if (buf_sz > req->m_res_content.m_length) {
                buf_sz = req->m_res_content.m_length;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: req %d: <== body %d data(left=%d)",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id, buf_sz, req->m_res_content.m_length - buf_sz);
            }

            if (net_http_req_input_body_consume_body_part(http_protocol, http_ep, req, endpoint, buf_sz) != 0) return -1;
            
            req->m_res_content.m_length -= buf_sz;
            if (req->m_res_content.m_length == 0) {
                if (net_http_req_input_body_set_complete(http_protocol, http_ep, req, endpoint, net_http_res_complete) != 0) return -1;
            }
        }
    }

    return 0;
}

static int net_http_req_input_body_encoding_trunked(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint)
{
    uint32_t buf_sz;
    
    while(
        req->m_res_state != net_http_res_state_completed
        && (buf_sz = net_endpoint_buf_size(endpoint, net_ep_buf_http_in)) > 0)
    {
        if (req->m_res_trunked.m_state == net_http_trunked_length) {
            char * data;
            uint32_t size;
            if (net_endpoint_buf_by_str(endpoint, net_ep_buf_http_in, "\r\n", (void**)&data, &size) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: req %d: <== trunked: search trunk len fail",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id);
                return -1;
            }

            if (data == NULL) {
                if(net_endpoint_buf_size(endpoint, net_ep_buf_forward) > 30) {
                    CPE_ERROR(
                        http_protocol->m_em, "http: %s: req %d: <== trunked: length linke too big! size=%d",
                        net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                        req->m_id,
                        net_endpoint_buf_size(endpoint, net_ep_buf_http_in));
                    return -1;
                }
                else {
                    return 0;
                }
            }

            char * endptr = NULL;
            req->m_res_trunked.m_length = (uint32_t)strtol(data, &endptr, 16);
            if (endptr == NULL || *endptr != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: req %d: <== trunk[%d]: length %s format error",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id,
                    req->m_res_trunked.m_count,
                    data);
                return -1;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: req %d: <== trunk[%d].length = 0x%s(%d)",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id,
                    req->m_res_trunked.m_count,
                    data,
                    req->m_res_trunked.m_length);
            }

            net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, size);

            req->m_res_trunked.m_state =
                req->m_res_trunked.m_length == 0
                ? net_http_trunked_complete
                : net_http_trunked_content;
        }
        else if (req->m_res_trunked.m_state == net_http_trunked_content) {
            if (buf_sz > req->m_res_trunked.m_length) {
                buf_sz = req->m_res_trunked.m_length;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: req %d: <== trunk[%d]: %d data(left=%d)",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id,
                    req->m_res_trunked.m_count,
                    buf_sz, req->m_res_trunked.m_length - buf_sz);
            }

            if (net_http_req_input_body_consume_body_part(http_protocol, http_ep, req, endpoint, buf_sz) != 0) return -1;
            
            req->m_res_trunked.m_length -= buf_sz;
            if (req->m_res_trunked.m_length == 0) {
                req->m_res_trunked.m_state = net_http_trunked_content_complete;
            }
        }
        else if (req->m_res_trunked.m_state == net_http_trunked_content_complete) {
            if (buf_sz < 2) return 0;
            
            char * data = NULL;
            net_endpoint_buf_peak_with_size(endpoint, net_ep_buf_http_in, 2, (void**)&data);
            assert(data);
            if (data[0] != '\r' || data[1] != '\n') {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: req %d: <== trunk[%d]: trunk rn mismatch, '%c', '%c'",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id,
                    req->m_res_trunked.m_count,
                    data[0], data[1]);
                return -1;
            }

            net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, 2);
            
            req->m_res_trunked.m_state = net_http_trunked_length;
            req->m_res_trunked.m_count++;
        }
        else {
            assert(req->m_res_trunked.m_state == net_http_trunked_complete);
            if (buf_sz < 2) return 0;
            
            char * data;
            net_endpoint_buf_peak_with_size(endpoint, net_ep_buf_http_in, 2, (void**)&data);
            assert(data);
            if (data[0] != '\r' || data[1] != '\n') {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: req %d: <== trunked: last rn mismach",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id);
                return -1;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: req %d: <== trunked: completed, trunk-count=%d",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    req->m_id,
                    req->m_res_trunked.m_count);
            }

            net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, 2);

            if (net_http_req_input_body_set_complete(http_protocol, http_ep, req, endpoint, net_http_res_complete) != 0) {
                return -1;
            }

            return 0;
        }
    }
    
    return 0;
}
