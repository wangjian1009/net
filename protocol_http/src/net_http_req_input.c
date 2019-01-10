#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_http_req_i.h"

static int net_http_endpoint_input_body_consume_body_part(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_endpoint_t endpoint, uint16_t buf_sz);

static int net_http_endpoint_input_body_encoding_none(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_endpoint_t endpoint);
static int net_http_endpoint_input_body_encoding_trunked(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_endpoint_t endpoint);

static void net_http_endpoint_do_parse_head_lines(char * data, char * lines[], uint8_t *line_count);
static int net_http_endpoint_do_parse_request_id(const char * tag, uint32_t * request_id, char * lines[], uint8_t line_count);


int net_http_endpoint_do_process(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_endpoint_t endpoint) {
    void * buf;
    uint32_t buf_size;

    if (http_ep->m_current_res.m_state == net_http_res_state_reading_head) {
        assert(http_ep->m_current_res.m_req == NULL);
        
        if (net_endpoint_buf_by_str(endpoint, net_ep_buf_http_in, "\r\n\r\n", &buf, &buf_size)) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: response: search sep fail",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint));
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
        
        if (net_endpoint_protocol_debug(endpoint) >= 2) {
            CPE_INFO(
                http_protocol->m_em, "http: %s: <== head\n%s",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint), buf);
        }

        char * head_lines[100];
        uint8_t head_line_count = CPE_ARRAY_SIZE(head_lines);
        net_http_endpoint_do_parse_head_lines(buf, head_lines, &head_line_count);

        net_http_req_t req;
        
        if (http_ep->m_request_id_tag) {
            uint32_t request_id;
            if (net_http_endpoint_do_parse_request_id(http_ep->m_request_id_tag, &request_id, head_lines, head_line_count) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: <== no request-id tag %s",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_request_id_tag);
                return -1;
            }

            TAILQ_FOREACH(req, &http_ep->m_reqs, m_next) {
                if (req->m_id == request_id) {
                    break;
                }
            }

            if (req == NULL) {
                CPE_INFO(
                    http_protocol->m_em,
                    "http: %s: req %d not exist, ignore!",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            }
        }
        else {
            req = TAILQ_FIRST(&http_ep->m_reqs);
            if (req == NULL) {
                CPE_ERROR(
                    http_protocol->m_em,
                    "http: %s: input without req or upgraded-processor!",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                return -1;
            }
        }
        
        uint8_t head_line_num = 0;
        for(; head_line_num < head_line_count; head_line_num++) {
            if (net_http_req_process_response_head_line(
                    http_protocol, http_ep, req, endpoint, head_lines[head_line_num], head_line_num) != 0)
            {
                return -1;
            }
        }
        
        net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, buf_size);

        http_ep->m_current_res.m_req = req;
        http_ep->m_current_res.m_state = net_http_res_state_reading_body;
    }

    if (http_ep->m_current_res.m_state == net_http_res_state_reading_body) {
        switch(http_ep->m_current_res.m_trans_encoding) {
        case net_http_trans_encoding_none:
            if (net_http_endpoint_input_body_encoding_none(http_protocol, http_ep, endpoint) != 0) return -1;
            break;
        case net_http_trans_encoding_trunked:
            if (net_http_endpoint_input_body_encoding_trunked(http_protocol, http_ep, endpoint) != 0) return -1;
            break;
        }
    }

    return 0;
}

static void net_http_endpoint_do_parse_head_lines(char * data, char * lines[], uint8_t *line_count) {
    uint8_t line_capacity = *line_count;

    *line_count = 0;
    char * line = data;
    char * sep = strstr(line, "\r\n");
    for(; *line_count < line_capacity && sep ; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        lines[(*line_count)++] = line;
    }

    if (*line_count < line_capacity && line[0]) {
        lines[(*line_count)++] = line;
    }
}

static int net_http_endpoint_do_parse_request_id(const char * tag, uint32_t * request_id, char * lines[], uint8_t line_count) {
    uint8_t i;

    for(i = 0; i < line_count; ++i) {
        char * line = lines[i];

        char * sep = strchr(line, ':');
        if (sep == NULL) continue;

        char * name = line;
        char * value = cpe_str_trim_head(sep + 1);

        sep = cpe_str_trim_tail(sep, line);

        char save = *sep;
        *sep = 0;

        if (strcasecmp(name, tag) == 0) {
            *request_id = atoi(value);
            *sep = save;
            return 0;
        }
        
        *sep = save;
    }
    
    return -1;
}

int net_http_req_process_response_head_line(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req,
    net_endpoint_t endpoint, char * line, uint32_t line_num)
{
    if (line_num == 0) {
        char * sep1 = strchr(line, ' ');
        char * sep2 = sep1 ? strchr(sep1 + 1, ' ') : NULL;
        if (sep1 == NULL || sep2 == NULL) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: parse head method: first line %s format error",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint), line);
            return -1;
        }

        char * version = line;
        *sep1 = 0;

        char * code = sep1 + 1;
        *sep2 = 0;

        char * msg = sep2 + 1;

        if (req) {
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
        }
        
        *sep1 = ' ';
        *sep2 = ' ';
    }
    else {
        char * sep = strchr(line, ':');
        if (sep == NULL) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: <== head line format error\n%s",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint), line);
            return -1;
        }

        char * name = line;
        char * value = cpe_str_trim_head(sep + 1);

        sep = cpe_str_trim_tail(sep, line);

        *sep = 0;

        if (strcasecmp(name, "Content-Length") == 0) {
            http_ep->m_current_res.m_trans_encoding = net_http_trans_encoding_none;
            char * endptr = NULL;
            http_ep->m_current_res.m_res_content.m_length = (uint32_t)strtol(value, &endptr, 10);
            if (endptr == NULL || *cpe_str_trim_head(endptr) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: <== Content-Length %s format error",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint), value);
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
                http_ep->m_current_res.m_trans_encoding = net_http_trans_encoding_trunked;
                http_ep->m_current_res.m_res_trunked.m_state = net_http_trunked_length;
                http_ep->m_current_res.m_res_trunked.m_count = 0;
                http_ep->m_current_res.m_res_trunked.m_length = 0;
            }
            else {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: Transfer-Encoding %s unknown",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint), value);
            }
        }
        
        if (req != NULL && !req->m_res_ignore && req->m_res_on_head) {
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

static int net_http_req_input_body_set_complete(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_endpoint_t endpoint, net_http_res_result_t result)
{
    http_ep->m_current_res.m_state = net_http_res_state_completed;

    net_http_req_t req = http_ep->m_current_res.m_req;
    if (req && !req->m_res_ignore && req->m_res_on_complete) {
        switch(req->m_res_on_complete(req->m_res_ctx, req, result)) {
        case net_http_res_op_success:
            break;
        case net_http_res_op_ignore:
            req->m_res_ignore = 1;
            http_ep->m_current_res.m_req = NULL;
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

static int net_http_endpoint_input_body_consume_body_part(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep,
    net_endpoint_t endpoint, uint16_t buf_sz)
{
    net_http_req_t req = http_ep->m_current_res.m_req;

    if (req == NULL) {
        net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, buf_sz);
        return 0;
    }
    
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

static int net_http_endpoint_input_body_encoding_none(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_endpoint_t endpoint) {
    if (http_ep->m_connection_type != net_http_connection_type_close && http_ep->m_current_res.m_res_content.m_length == 0) {
        if (net_http_req_input_body_set_complete(http_protocol, http_ep, endpoint, net_http_res_complete) != 0) return -1;
        return 0;
    }
    
    while(http_ep->m_current_res.m_state != net_http_res_state_completed) {
        uint32_t buf_sz = net_endpoint_buf_size(endpoint, net_ep_buf_http_in);
        if (buf_sz == 0) break;
        
        if (http_ep->m_current_res.m_res_content.m_length == 0) {
            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: <== body %d data",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint), buf_sz);
            }

            if (net_http_endpoint_input_body_consume_body_part(http_protocol, http_ep, endpoint, buf_sz) != 0) return -1;
        }
        else {
            if (buf_sz > http_ep->m_current_res.m_res_content.m_length) {
                buf_sz = http_ep->m_current_res.m_res_content.m_length;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: <== body %d data(left=%d)",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    buf_sz, http_ep->m_current_res.m_res_content.m_length - buf_sz);
            }

            if (net_http_endpoint_input_body_consume_body_part(http_protocol, http_ep, endpoint, buf_sz) != 0) return -1;
            
            http_ep->m_current_res.m_res_content.m_length -= buf_sz;
            if (http_ep->m_current_res.m_res_content.m_length == 0) {
                if (net_http_req_input_body_set_complete(http_protocol, http_ep, endpoint, net_http_res_complete) != 0) return -1;
            }
        }
    }

    return 0;
}

static int net_http_endpoint_input_body_encoding_trunked(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_endpoint_t endpoint) {
    uint32_t buf_sz;
    
    while(
        http_ep->m_current_res.m_state != net_http_res_state_completed
        && (buf_sz = net_endpoint_buf_size(endpoint, net_ep_buf_http_in)) > 0)
    {
        if (http_ep->m_current_res.m_res_trunked.m_state == net_http_trunked_length) {
            char * data;
            uint32_t size;
            if (net_endpoint_buf_by_str(endpoint, net_ep_buf_http_in, "\r\n", (void**)&data, &size) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: <== trunked: search trunk len fail",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint));
                return -1;
            }

            if (data == NULL) {
                if(net_endpoint_buf_size(endpoint, net_ep_buf_forward) > 30) {
                    CPE_ERROR(
                        http_protocol->m_em, "http: %s: <== trunked: length linke too big! size=%d",
                        net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                        net_endpoint_buf_size(endpoint, net_ep_buf_http_in));
                    return -1;
                }
                else {
                    return 0;
                }
            }

            char * endptr = NULL;
            http_ep->m_current_res.m_res_trunked.m_length = (uint32_t)strtol(data, &endptr, 16);
            if (endptr == NULL || *endptr != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: <== trunk[%d]: length %s format error",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_current_res.m_res_trunked.m_count,
                    data);
                return -1;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: <== trunk[%d].length = 0x%s(%d)",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_current_res.m_res_trunked.m_count,
                    data,
                    http_ep->m_current_res.m_res_trunked.m_length);
            }

            net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, size);

            http_ep->m_current_res.m_res_trunked.m_state =
                http_ep->m_current_res.m_res_trunked.m_length == 0
                ? net_http_trunked_complete
                : net_http_trunked_content;
        }
        else if (http_ep->m_current_res.m_res_trunked.m_state == net_http_trunked_content) {
            if (buf_sz > http_ep->m_current_res.m_res_trunked.m_length) {
                buf_sz = http_ep->m_current_res.m_res_trunked.m_length;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: <== trunk[%d]: %d data(left=%d)",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_current_res.m_res_trunked.m_count,
                    buf_sz, http_ep->m_current_res.m_res_trunked.m_length - buf_sz);
            }

            if (net_http_endpoint_input_body_consume_body_part(http_protocol, http_ep, endpoint, buf_sz) != 0) return -1;
            
            http_ep->m_current_res.m_res_trunked.m_length -= buf_sz;
            if (http_ep->m_current_res.m_res_trunked.m_length == 0) {
                http_ep->m_current_res.m_res_trunked.m_state = net_http_trunked_content_complete;
            }
        }
        else if (http_ep->m_current_res.m_res_trunked.m_state == net_http_trunked_content_complete) {
            if (buf_sz < 2) return 0;
            
            char * data = NULL;
            net_endpoint_buf_peak_with_size(endpoint, net_ep_buf_http_in, 2, (void**)&data);
            assert(data);
            if (data[0] != '\r' || data[1] != '\n') {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: <== trunk[%d]: trunk rn mismatch, '%c', '%c'",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_current_res.m_res_trunked.m_count,
                    data[0], data[1]);
                return -1;
            }

            net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, 2);
            
            http_ep->m_current_res.m_res_trunked.m_state = net_http_trunked_length;
            http_ep->m_current_res.m_res_trunked.m_count++;
        }
        else {
            assert(http_ep->m_current_res.m_res_trunked.m_state == net_http_trunked_complete);
            if (buf_sz < 2) return 0;
            
            char * data;
            net_endpoint_buf_peak_with_size(endpoint, net_ep_buf_http_in, 2, (void**)&data);
            assert(data);
            if (data[0] != '\r' || data[1] != '\n') {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: <== trunked: last rn mismach",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint));
                return -1;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http: %s: <== trunked: completed, trunk-count=%d",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_current_res.m_res_trunked.m_count);
            }

            net_endpoint_buf_consume(endpoint, net_ep_buf_http_in, 2);

            if (net_http_req_input_body_set_complete(http_protocol, http_ep, endpoint, net_http_res_complete) != 0) {
                return -1;
            }

            return 0;
        }
    }
    
    return 0;
}
