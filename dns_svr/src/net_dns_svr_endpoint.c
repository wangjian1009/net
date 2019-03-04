#include "cpe/pal/pal_platform.h"
#include "net_endpoint.h"
#include "net_dns_svr_endpoint_i.h"
#include "net_dns_svr_itf_i.h"
#include "net_dns_svr_query_i.h"

net_dns_svr_endpoint_t net_dns_svr_endpoint_get(net_endpoint_t base_endpoint) {
    return net_endpoint_data(base_endpoint);
}

void net_dns_svr_endpoint_set_itf(net_dns_svr_endpoint_t endpoint, net_dns_svr_itf_t dns_itf) {
    endpoint->m_itf = dns_itf;
}

int net_dns_svr_endpoint_init(net_endpoint_t base_endpoint) {
    net_dns_svr_endpoint_t dns_ep = net_endpoint_protocol_data(base_endpoint);
    dns_ep->m_itf = NULL;
    return 0;
}

void net_dns_svr_endpoint_fini(net_endpoint_t base_endpoint) {
    net_dns_svr_endpoint_t dns_ep = net_endpoint_protocol_data(base_endpoint);
    if (dns_ep->m_itf) {
        net_dns_svr_query_t query, next_query;
        for(query = TAILQ_FIRST(&dns_ep->m_itf->m_querys); query; query = next_query) {
            next_query = TAILQ_NEXT(query, m_next);

            if (query->m_endpoint == base_endpoint) {
                net_dns_svr_query_free(query);
            }
        }
    }
}

static int net_dns_svr_endpoint_process_data(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf)
{
    net_dns_svr_endpoint_t dns_ep = net_endpoint_protocol_data(base_endpoint);
    net_dns_svr_itf_t dns_itf = dns_ep->m_itf;
    net_dns_svr_t svr = dns_itf->m_svr;

    while(1) {
        uint32_t input_sz = net_endpoint_buf_size(from_ep, from_buf);
        CPE_ERROR(
            svr->m_em, "dns-svr: %s: <-- xxxxx buf=%s, sz=%d!",
            net_endpoint_dump(net_dns_svr_tmp_buffer(svr), base_endpoint), net_endpoint_buf_type_str(from_buf), input_sz);

        if (input_sz < 2) return 0;

        void * input;
        if (net_endpoint_buf_peak_with_size(from_ep, from_buf, input_sz, &input) != 0) {
            CPE_ERROR(
                svr->m_em, "dns-svr: %s: <-- get input buf fail, sz=%d!",
                net_endpoint_dump(net_dns_svr_tmp_buffer(svr), base_endpoint), input_sz);
            return -1;
        }

        uint16_t req_sz;
        CPE_COPY_NTOH16(&req_sz, input);

        uint32_t all_req_sz = req_sz + (uint32_t)sizeof(req_sz);
        if (all_req_sz > input_sz) return 0;

        input = ((char*)input) + sizeof(req_sz);

        if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
            CPE_INFO(
                svr->m_em, "dns-svr: %s: tcp <<< (%d) %s",
                net_endpoint_dump(&svr->m_data_buffer, base_endpoint),
                req_sz,
                net_dns_svr_req_dump(svr, net_dns_svr_tmp_buffer(svr), input, (uint32_t)req_sz));
        }

        net_dns_svr_query_t query = net_dns_svr_query_parse_request(dns_itf, input, (uint32_t)req_sz);
        if (query == NULL) {
            CPE_ERROR(
                svr->m_em, "dns-svr: %s: <<< parse data fail, req-sz=%d",
                net_endpoint_dump(net_dns_svr_tmp_buffer(svr), base_endpoint), req_sz);
            return -1;
        }

        net_endpoint_buf_consume(from_ep, from_buf, all_req_sz);

        /*处理query */
        query->m_endpoint = base_endpoint;
        
        if (net_dns_svr_query_start(query) != 0) {
            CPE_ERROR(
                svr->m_em, "dns-svr: %s: <<< start query fail",
                net_endpoint_dump(net_dns_svr_tmp_buffer(svr), base_endpoint));
            net_dns_svr_query_free(query);
            continue;
        }

        if (query->m_runing_entry_count == 0) {
            if (net_dns_svr_itf_send_response(dns_itf, query) != 0) {
                net_dns_svr_query_free(query);
                return -1;
            }
            
            net_dns_svr_query_free(query);
        }

        return 0;
    }

    return 0;
}

int net_dns_svr_endpoint_input(net_endpoint_t base_endpoint) {
    return net_dns_svr_endpoint_process_data(base_endpoint, base_endpoint, net_ep_buf_read);
}

int net_dns_svr_endpoint_forward(net_endpoint_t base_endpoint, net_endpoint_t from) {
    return net_dns_svr_endpoint_process_data(base_endpoint, from, net_ep_buf_forward);
}
