#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_http2_req_i.h"

int net_http2_req_add_res_head(
    net_http2_req_t http_req,
    const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len)
{
    net_http2_endpoint_t http_ep = http_req->m_endpoint;
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    if (http_req->m_res_head_count >= http_req->m_res_head_capacity) {
        uint16_t new_capacity = http_req->m_res_head_capacity < 8 ? 8 : http_req->m_res_head_capacity * 2;
        struct net_http2_req_pair * new_headers =
            mem_alloc(protocol->m_alloc, sizeof(struct net_http2_req_pair) * new_capacity);
        if (new_headers == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: req: add header: alloc nv faild, capacity=%d!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), new_capacity);
            return -1;
        }

        if (http_req->m_res_headers) {
            memcpy(new_headers, http_req->m_res_headers, sizeof(struct net_http2_req_pair) * http_req->m_res_head_count);
            mem_free(protocol->m_alloc, http_req->m_res_headers);
        }

        http_req->m_res_headers = new_headers;
        http_req->m_res_head_capacity = new_capacity;
    }
    
    struct net_http2_req_pair * nv = http_req->m_res_headers + http_req->m_res_head_count;
    nv->m_name = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_name, name_len);
    if (nv->m_name == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: req: add res header: dup name %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), attr_name);
        return -1;
    }

    nv->m_value = cpe_str_mem_dup_len(protocol->m_alloc, attr_value, value_len);
    if (nv->m_value == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: req: add res header: dup value %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), attr_value);
        mem_free(protocol->m_alloc, nv->m_name);
        return -1;
    }

    http_req->m_res_head_count++;
    return 0;
}

const char * net_http2_req_find_res_header(net_http2_req_t req, const char * name) {
    uint16_t i;

    for(i = 0; i < req->m_res_head_count; i++) {
        struct net_http2_req_pair * header = req->m_res_headers + i;
        if (strcmp(header->m_name, name) == 0) return header->m_value;
    }

    return NULL;
}
