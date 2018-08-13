#include "net_address.h"
#include "net_dgram.h"
#include "net_acceptor.h"
#include "net_dns_svr_itf_i.h"
#include "net_dns_svr_endpoint_i.h"
#include "net_dns_svr_query_i.h"

static void net_dns_svr_dgram_process(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);
static int net_dns_svr_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint);

net_dns_svr_itf_t
net_dns_svr_itf_create(net_dns_svr_t dns_svr, net_dns_svr_itf_type_t type, net_address_t address, net_driver_t driver) {
    net_dns_svr_itf_t dns_itf = mem_alloc(dns_svr->m_alloc, sizeof(struct net_dns_svr_itf));
    if (dns_itf == NULL) {
        CPE_ERROR(dns_svr->m_em, "net: dns: alloc dns svr itf fail");
        return NULL;
    }

    dns_itf->m_svr = dns_svr;
    dns_itf->m_type = type;

    switch(type) {
    case net_dns_svr_itf_udp:
        dns_itf->m_dgram = net_dgram_create(driver, address, net_dns_svr_dgram_process, dns_itf);
        if (dns_itf->m_dgram == NULL) {
            CPE_ERROR(dns_svr->m_em, "net: dns: create dgram fail");
            return NULL;
        }
        break;
    case net_dns_svr_itf_tcp:
        dns_itf->m_acceptor = net_acceptor_create(
            driver, dns_svr->m_dns_protocol,
            address, 0, 0,
            net_dns_svr_acceptor_on_new_endpoint, dns_itf);
        if (dns_itf->m_acceptor == NULL) {
            CPE_ERROR(dns_svr->m_em, "net: dns: create acceptor fail");
            return NULL;
        }
        break;
    }

    TAILQ_INIT(&dns_itf->m_querys);
    
    TAILQ_INSERT_TAIL(&dns_svr->m_itfs, dns_itf, m_next);

    if (dns_svr->m_debug) {
        CPE_INFO(
            dns_svr->m_em, "net: dns: itf %s %s created",
            net_dns_svr_itf_type_str(type),
            net_address_dump(net_dns_svr_tmp_buffer(dns_svr), address));
    }
    
    return dns_itf;
}

void net_dns_svr_itf_free(net_dns_svr_itf_t dns_itf) {
    net_dns_svr_t dns_svr = dns_itf->m_svr;

    while(!TAILQ_EMPTY(&dns_itf->m_querys)) {
        net_dns_svr_query_free(TAILQ_FIRST(&dns_itf->m_querys));
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

static void net_dns_svr_dgram_process(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source) {
    net_dns_svr_itf_t dns_itf = ctx;

    CPE_ERROR(dns_itf->m_svr->m_em, "xxx: receive dns query!");
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