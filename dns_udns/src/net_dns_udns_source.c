#include "assert.h"
#include "udns.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream.h"
#include "net_driver.h"
#include "net_watcher.h"
#include "net_timer.h"
#include "net_dns_manage.h"
#include "net_dns_source.h"
#include "net_dns_udns_source_i.h"
#include "net_dns_udns_source_ctx_i.h"

int net_dns_udns_source_init(net_dns_source_t source);
void net_dns_udns_source_fini(net_dns_source_t source);
void net_dns_udns_source_dump(write_stream_t ws, net_dns_source_t source);
void net_dns_udns_source_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
void net_dns_udns_source_timeout_cb(net_timer_t timer, void * ctx);
void net_dns_udns_source_dbg(
    int code, const struct sockaddr *sa, unsigned salen,
    dnscc_t *pkt, int plen,
    const struct dns_query *q, void *data);

void net_dns_udns_source_timer_setup_cb(struct dns_ctx *, int, void *);

net_dns_udns_source_t
net_dns_udns_source_create(
    mem_allocrator_t alloc, error_monitor_t em, uint8_t debug,
    net_dns_manage_t manage, net_driver_t driver)
{
    net_dns_source_t source =
        net_dns_source_create(
            manage,
            sizeof(struct net_dns_udns_source),
            net_dns_udns_source_init,
            net_dns_udns_source_fini,
            net_dns_udns_source_dump,
            sizeof(struct net_dns_udns_source_ctx),
            net_dns_udns_source_ctx_init,
            net_dns_udns_source_ctx_fini,
            net_dns_udns_source_ctx_start,
            net_dns_udns_source_ctx_cancel);
    if (source == NULL) return NULL;

    net_dns_udns_source_t udns = net_dns_source_data(source);

    udns->m_alloc = alloc;
    udns->m_em = em;
    udns->m_debug = debug;
    udns->m_manage = manage;
    udns->m_driver = driver;
    udns->m_watcher = NULL;
    udns->m_timeout = net_timer_auto_create(net_driver_schedule(driver), net_dns_udns_source_timeout_cb, udns);
    if (udns->m_timeout == NULL) {
        CPE_ERROR(em, "udns: create timer fail!");
        net_dns_source_free(source);
        return NULL;
    }
    
    dns_reset(NULL);
    udns->m_dns_ctx = dns_new(NULL);
    if (udns->m_dns_ctx == NULL) {
        CPE_ERROR(em, "udns: dns_new fail!");
        net_dns_source_free(source);
        return NULL;
    }

    return udns;
}

void net_dns_udns_source_free(net_dns_udns_source_t udns) {
    net_dns_source_free(net_dns_source_from_data(udns));
}

net_dns_udns_source_t net_dns_udns_source_from_source(net_dns_source_t source) {
    return net_dns_source_init(source) == net_dns_udns_source_init
        ? net_dns_source_data(source)
        : NULL;
}

void net_dns_udns_source_reset(net_dns_udns_source_t udns) {
    if (udns->m_debug) {
        CPE_INFO(udns->m_em, "udns: rested!");
    }

    if (udns->m_watcher) {
        net_watcher_free(udns->m_watcher);
        udns->m_watcher = NULL;
    }
    
    dns_reset(udns->m_dns_ctx);
}

int net_dns_udns_source_add_server(net_dns_udns_source_t udns, net_address_t address) {
    if (udns->m_debug) {
        CPE_INFO(udns->m_em, "udns: rested!");
    }
    
    return 0;
}

int net_dns_udns_source_start(net_dns_udns_source_t udns) {
    dns_set_dbgfn(udns->m_dns_ctx, net_dns_udns_source_dbg);

    int sockfd = dns_open(udns->m_dns_ctx);
    if (sockfd < 0) {
        CPE_ERROR(udns->m_em, "udns: start: open DNS resolver socket fail");
        return -1;
    }

    assert(udns->m_watcher == NULL);
    udns->m_watcher = net_watcher_create(udns->m_driver, sockfd, udns, net_dns_udns_source_rw_cb);
    if (udns->m_watcher == NULL) {
        CPE_ERROR(udns->m_em, "udns: start: open DNS resolver socket fail");
        dns_close(udns->m_dns_ctx);
        return -1;
    }
    net_watcher_expect_read(udns->m_watcher);

    dns_set_tmcbck(udns->m_dns_ctx, net_dns_udns_source_timer_setup_cb, udns);
    
    return 0;
}

int net_dns_udns_source_init(net_dns_source_t source) {
    net_dns_udns_source_t udns = net_dns_source_data(source);

    bzero(udns, sizeof(*udns));

    return 0;
}

void net_dns_udns_source_fini(net_dns_source_t source) {
    net_dns_udns_source_t udns = net_dns_source_data(source);

    if (udns->m_watcher) {
        net_watcher_free(udns->m_watcher);
        udns->m_watcher = NULL;
    }
    
    if (udns->m_dns_ctx) {
        dns_free(udns->m_dns_ctx);
        udns->m_dns_ctx = NULL;
    }

    if (udns->m_timeout) {
        net_timer_free(udns->m_timeout);
        udns->m_timeout = NULL;
    }
}

void net_dns_udns_source_dump(write_stream_t ws, net_dns_source_t source) {
    net_dns_udns_source_t udns = net_dns_source_data(source);
    
    stream_printf(ws, "udns[");
    stream_printf(ws, "]");
}

void net_dns_udns_source_dbg(
    int code, const struct sockaddr *sa, unsigned salen, dnscc_t *pkt, int plen, const struct dns_query *q, void *data)
{
}

void net_dns_udns_source_timer_setup_cb(struct dns_ctx *ctx, int timeout, void *data) {
    net_dns_udns_source_t udns = data;

    if (ctx != NULL && timeout >= 0) {
        if (udns->m_debug) {
            CPE_INFO(udns->m_em, "udns: timer active, timeout=%d!", timeout);
        }
        
        net_timer_active(udns->m_timeout, timeout);
    }
    else {
        net_timer_cancel(udns->m_timeout);
    }
}

void net_dns_udns_source_timeout_cb(net_timer_t timer, void * ctx) {
    net_dns_udns_source_t udns = ctx;
    dns_timeouts(udns->m_dns_ctx, 30, time(0));
}

void net_dns_udns_source_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_dns_udns_source_t udns = ctx;

    if (do_read) {
        dns_ioevent(udns->m_dns_ctx, time(0));
    }
}

