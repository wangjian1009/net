#include "assert.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_dgram.h"
#include "net_acceptor.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_dns_svr_itf_i.h"
#include "net_dns_svr_endpoint_i.h"
#include "net_dns_svr_query_i.h"

static void net_dns_svr_dgram_process(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);
static int net_dns_svr_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint);

net_dns_svr_itf_t
net_dns_svr_itf_create(net_dns_svr_t dns_svr, net_dns_svr_itf_type_t type, net_address_t address, net_driver_t driver) {
    net_dns_svr_itf_t dns_itf = mem_alloc(dns_svr->m_alloc, sizeof(struct net_dns_svr_itf));
    if (dns_itf == NULL) {
        CPE_ERROR(dns_svr->m_em, "dns-svr: alloc dns svr itf fail");
        return NULL;
    }

    dns_itf->m_svr = dns_svr;
    dns_itf->m_type = type;
    dns_itf->m_query_policy = NULL;

    switch(type) {
    case net_dns_svr_itf_udp:
        dns_itf->m_dgram = net_dgram_create(driver, address, net_dns_svr_dgram_process, dns_itf);
        if (dns_itf->m_dgram == NULL) {
            CPE_ERROR(dns_svr->m_em, "dns-svr: create dgram fail");
            return NULL;
        }
        break;
    case net_dns_svr_itf_tcp:
        dns_itf->m_acceptor = net_acceptor_create(
            driver, dns_svr->m_dns_protocol,
            address, 0,
            net_dns_svr_acceptor_on_new_endpoint, dns_itf);
        if (dns_itf->m_acceptor == NULL) {
            CPE_ERROR(dns_svr->m_em, "dns-svr: create acceptor fail");
            return NULL;
        }
        break;
    }

    TAILQ_INIT(&dns_itf->m_querys);
    
    TAILQ_INSERT_TAIL(&dns_svr->m_itfs, dns_itf, m_next);

    if (dns_svr->m_debug) {
        CPE_INFO(
            dns_svr->m_em, "dns-svr: itf %s %s created",
            net_dns_svr_itf_type_str(type),
            net_address_dump(net_dns_svr_tmp_buffer(dns_svr), address));
    }
    
    return dns_itf;
}

void net_dns_svr_itf_free(net_dns_svr_itf_t dns_itf) {
    net_dns_svr_t dns_svr = dns_itf->m_svr;

    if (dns_svr->m_debug) {
        CPE_INFO(
            dns_svr->m_em, "dns-svr: itf %s %s free",
            net_dns_svr_itf_type_str(dns_itf->m_type),
            net_address_dump(net_dns_svr_tmp_buffer(dns_svr), net_dns_itf_address(dns_itf)));
    }
    
    while(!TAILQ_EMPTY(&dns_itf->m_querys)) {
        net_dns_svr_query_free(TAILQ_FIRST(&dns_itf->m_querys));
    }

    if (dns_itf->m_query_policy) {
        mem_free(dns_svr->m_alloc, dns_itf->m_query_policy);
        dns_itf->m_query_policy = NULL;
    }

    switch(dns_itf->m_type) {
    case net_dns_svr_itf_udp:
        net_dgram_free(dns_itf->m_dgram);
        dns_itf->m_dgram = NULL;
        break;
    case net_dns_svr_itf_tcp:
        net_acceptor_free(dns_itf->m_acceptor);
        dns_itf->m_acceptor = NULL;
        break;
    }
    
    TAILQ_REMOVE(&dns_svr->m_itfs, dns_itf, m_next);
    mem_free(dns_svr->m_alloc, dns_itf);
}

net_dns_svr_t net_dns_itf_svr(net_dns_svr_itf_t dns_itf) {
    return dns_itf->m_svr;
}

net_dns_svr_itf_type_t net_dns_itf_type(net_dns_svr_itf_t dns_itf) {
    return dns_itf->m_type;
}

net_address_t net_dns_itf_address(net_dns_svr_itf_t dns_itf) {
    switch(dns_itf->m_type) {
    case net_dns_svr_itf_udp:
        return net_dgram_address(dns_itf->m_dgram);
    case net_dns_svr_itf_tcp:
        return net_acceptor_address(dns_itf->m_acceptor);
    }
}

net_driver_t net_dns_itf_driver(net_dns_svr_itf_t dns_itf) {
    switch(dns_itf->m_type) {
    case net_dns_svr_itf_udp:
        return net_dgram_driver(dns_itf->m_dgram);
    case net_dns_svr_itf_tcp:
        return net_acceptor_driver(dns_itf->m_acceptor);
    }
}

const char * net_dns_itf_query_policy(net_dns_svr_itf_t dns_itf) {
    return dns_itf->m_query_policy;
}

int net_dns_itf_set_query_policy(net_dns_svr_itf_t dns_itf, const char * policy) {
    net_dns_svr_t dns_svr = dns_itf->m_svr;
    char * new_query_policy = NULL;

    if (policy) {
        new_query_policy = cpe_str_mem_dup(dns_svr->m_alloc, policy);
        if (new_query_policy == NULL) {
            CPE_ERROR(dns_svr->m_em, "dns-svr: dup policy %s fail", policy);
            return -1;
        }
    }

    if (dns_itf->m_query_policy) {
        mem_free(dns_svr->m_alloc, dns_itf->m_query_policy);
    }
    dns_itf->m_query_policy = new_query_policy;

    if (dns_svr->m_debug) {
        CPE_INFO(dns_svr->m_em, "dns-svr: policy ==> %s", dns_itf->m_query_policy ? dns_itf->m_query_policy : "N/A");
    }
    
    return 0;
}


int net_dns_svr_itf_send_response(net_dns_svr_itf_t dns_itf, net_dns_svr_query_t query) {
    net_dns_svr_t svr = dns_itf->m_svr;
    
    switch(dns_itf->m_type) {
    case net_dns_svr_itf_udp: {
        char buf[1500];
        uint32_t capacity = net_dns_svr_query_calc_response_size(query);
        if (capacity > sizeof(buf)) {
            CPE_ERROR(svr->m_em, "dns-svr: response size %d overflow!", capacity);
            return -1;
        }

        int rv = net_dns_svr_query_build_response(query, buf, capacity);
        if (rv < 0) {
            CPE_ERROR(svr->m_em, "dns-svr: build response fail!");
            return -1;
        }
        assert((uint32_t)rv <= capacity);

        if (net_dgram_protocol_debug(dns_itf->m_dgram, query->m_source_addr) >= 2 || net_protocol_debug(svr->m_dns_protocol) >= 2) {
            char address_buf[128];
            cpe_str_dup(address_buf, sizeof(address_buf), net_address_dump(net_dns_svr_tmp_buffer(svr), net_dgram_address(dns_itf->m_dgram)));
            CPE_INFO(
                svr->m_em, "dns-svr: [%s]: udp >>> %s",
                address_buf,
                net_dns_svr_req_dump(svr, net_dns_svr_tmp_buffer(svr), buf, (uint32_t)rv));
        }

        if (net_dgram_send(dns_itf->m_dgram, query->m_source_addr, buf, (uint32_t)rv) < 0) {
            CPE_ERROR(svr->m_em, "dns-svr: send response fail!");
            return -1;
        }

        return 0;
    }
    case net_dns_svr_itf_tcp: {
        assert(query->m_endpoint);
        
        uint32_t capacity = net_dns_svr_query_calc_response_size(query);
        capacity += sizeof(uint16_t); /*长度字段 */
        
        void * output_buf = net_endpoint_buf_alloc(query->m_endpoint, &capacity);
        if (output_buf == NULL) {
            CPE_ERROR(
                svr->m_em, "dns-svr: %s: tcp >>> alloc buf fail, capacity=%d!",
                net_endpoint_dump(net_dns_svr_tmp_buffer(svr), query->m_endpoint), capacity);
            return -1;
        }

        void * content_buf = ((uint8_t*)output_buf) + sizeof(uint16_t);
        int rv = net_dns_svr_query_build_response(query, content_buf, capacity - sizeof(uint16_t));
        if (rv < 0) {
            CPE_ERROR(
                svr->m_em, "dns-svr: %s: tcp >>> build response fail!",
                net_endpoint_dump(net_dns_svr_tmp_buffer(svr), query->m_endpoint));
            return -1;
        }
        assert((uint32_t)rv <= capacity - sizeof(uint16_t));

        uint16_t req_sz = (uint16_t)rv;
        CPE_COPY_HTON16(output_buf, &req_sz);

        if (net_endpoint_protocol_debug(query->m_endpoint) >= 2) {
            char address_buf[128];
            cpe_str_dup(
                address_buf, sizeof(address_buf),
                net_endpoint_dump(net_dns_svr_tmp_buffer(svr), query->m_endpoint));
            CPE_INFO(
                svr->m_em, "dns-svr: %s: tcp >>> (%d) %s",
                address_buf,
                req_sz,
                net_dns_svr_req_dump(svr, net_dns_svr_tmp_buffer(svr), content_buf, req_sz));
        }

        uint32_t total_sz = sizeof(uint16_t) + req_sz;
        if (net_endpoint_buf_supply(query->m_endpoint, net_ep_buf_write, total_sz) != 0) {
            CPE_ERROR(
                svr->m_em, "dns-svr: %s: tcp >>> supply fail, sz=%d!",
                net_endpoint_dump(net_dns_svr_tmp_buffer(svr), query->m_endpoint), total_sz);
            return -1;
        }
        
        return 0;
    }
    }
}

static void net_dns_svr_dgram_process(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source) {
    net_dns_svr_itf_t dns_itf = ctx;
    net_dns_svr_t svr = dns_itf->m_svr;

    if (net_dgram_protocol_debug(dgram, source) >= 2 || net_protocol_debug(svr->m_dns_protocol) >= 2) {
        char address_buf[128];
        cpe_str_dup(address_buf, sizeof(address_buf), net_address_dump(net_dns_svr_tmp_buffer(svr), net_dgram_address(dgram)));
        CPE_INFO(
            svr->m_em, "dns-svr: [%s]: udp <<< %s",
            address_buf,
            net_dns_svr_req_dump(svr, net_dns_svr_tmp_buffer(svr), (char const *)data, (uint32_t)data_size));
    }
    
    net_dns_svr_query_t query = net_dns_svr_query_parse_request(dns_itf, data, (uint32_t)data_size);
    if (query == NULL) return;

    if (net_dns_svr_query_set_source_addr(query, source) != 0) {
        CPE_ERROR(svr->m_em, "dns-svr: set source addr fail!");
        net_dns_svr_query_free(query);
        return;
    }
    
    if (net_dns_svr_query_start(query) != 0) {
        CPE_ERROR(svr->m_em, "dns-svr: start query fail!");
        net_dns_svr_query_free(query);
        return;
    }

    if (query->m_runing_entry_count == 0) {
        net_dns_svr_itf_send_response(dns_itf, query);
        net_dns_svr_query_free(query);
    }
}

static int net_dns_svr_acceptor_on_new_endpoint(void * ctx, net_endpoint_t base_endpoint) {
    net_dns_svr_itf_t dns_itf = ctx;

    net_dns_svr_endpoint_t dns_ep = net_dns_svr_endpoint_get(base_endpoint);
    net_dns_svr_endpoint_set_itf(dns_ep, dns_itf);
    
    return 0;
}

const char * net_dns_svr_itf_type_str(net_dns_svr_itf_type_t type) {
    switch(type) {
    case net_dns_svr_itf_udp:
        return "udp";
    case net_dns_svr_itf_tcp:
        return "tcp";
    }
}
