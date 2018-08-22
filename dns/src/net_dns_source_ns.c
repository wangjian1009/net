#include "cpe/pal/pal_platform.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/time_utils.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_dgram.h"
#include "net_timer.h"
#include "net_dns_task.h"
#include "net_dns_task_ctx.h"
#include "net_dns_source_ns_i.h"
#include "net_dns_source_i.h"
#include "net_dns_ns_cli_endpoint_i.h"
#include "net_dns_ns_pro.h"
#include "net_dns_ns_parser.h"

static int net_dns_source_ns_init(net_dns_source_t source);
static void net_dns_source_ns_fini(net_dns_source_t source);
static void net_dns_source_ns_dump(write_stream_t ws, net_dns_source_t source);

static void net_dns_source_ns_dgram_input(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t from);

static int net_dns_source_ns_dgram_output(
    net_dns_manage_t manage, net_dns_source_ns_t ns, void const * data, uint16_t buf_size);

static int net_dns_source_ns_tcp_output(
    net_dns_manage_t manage, net_dns_source_ns_t ns, void const * data, uint16_t buf_size);

static int net_dns_source_ns_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static void net_dns_source_ns_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static int net_dns_source_ns_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static void net_dns_source_ns_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

net_dns_source_ns_t
net_dns_source_ns_create(net_dns_manage_t manage, net_driver_t driver, net_address_t addr, uint8_t is_own) {
    net_dns_source_t source =
        net_dns_source_create(
            manage,
            sizeof(struct net_dns_source_ns),
            net_dns_source_ns_init,
            net_dns_source_ns_fini,
            net_dns_source_ns_dump,
            sizeof(struct net_dns_source_ns_ctx),
            net_dns_source_ns_ctx_init,
            net_dns_source_ns_ctx_fini,
            net_dns_source_ns_ctx_start,
            net_dns_source_ns_ctx_cancel);
    if (source == NULL) return NULL;

    net_dns_source_ns_t ns = net_dns_source_data(source);

    ns->m_driver = driver;
    
    if (is_own) {
        ns->m_address = addr;
    }
    else {
        ns->m_address = net_address_copy(manage->m_schedule, addr);
        if (ns->m_address == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: ns: copy address fail!");
            net_dns_source_free(source);
            return NULL;
        }
    }

    if (net_address_port(ns->m_address) == 0) {
        net_address_set_port(ns->m_address, 53);
    }

    if (manage->m_debug) {
        CPE_INFO(
            manage->m_em, "dns-cli: ns %s created!",
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), source));
    }
    
    return ns;
}

void net_dns_source_ns_free(net_dns_source_ns_t ns) {
    net_dns_source_free(net_dns_source_from_data(ns));
}

net_dns_source_ns_t net_dns_source_ns_find(net_dns_manage_t manage, net_address_t addr) {
    net_dns_source_t source;

    uint16_t port = net_address_port(addr);
    if (port == 0) port = 53; 
    
    TAILQ_FOREACH(source, &manage->m_sources, m_next) {
        if (source->m_init != net_dns_source_ns_init) continue;

        net_dns_source_ns_t ns = net_dns_source_data(source);

        if (net_address_cmp_without_port(ns->m_address, addr) != 0) continue;
        if (net_address_port(ns->m_address) != port) continue;

        return ns;
    }

    return 0;
}

net_dns_source_ns_t net_dns_source_ns_from_source(net_dns_source_t source) {
    return source->m_init == net_dns_source_ns_init
        ? net_dns_source_data(source)
        : NULL;
}

net_address_t net_dns_source_ns_address(net_dns_source_ns_t ns) {
    return ns->m_address;
}

net_dns_ns_trans_type_t net_dns_source_ns_trans_type(net_dns_source_ns_t ns) {
    return ns->m_trans_type;
}

void net_dns_source_ns_set_trans_type(net_dns_source_ns_t ns, net_dns_ns_trans_type_t trans_type) {
    ns->m_trans_type = trans_type;
}

void net_dns_source_ns_set_tcp_connect(
    net_dns_source_ns_t ns, net_endpoint_prepare_connect_fun_t tcp_connect, void * tcp_connect_ctx)
{
    ns->m_tcp_connect = tcp_connect;
    ns->m_tcp_connect_ctx = tcp_connect_ctx;
}

static int net_dns_source_ns_init(net_dns_source_t source) {
    net_dns_source_ns_t ns = net_dns_source_data(source);

    ns->m_driver = NULL;
    ns->m_address = NULL;
    ns->m_trans_type = net_dns_trans_udp;
    ns->m_tcp_connect = NULL;
    ns->m_tcp_connect_ctx = NULL;
    ns->m_dgram = NULL;
    ns->m_tcp_cli = NULL;
    ns->m_retry_count = 0;
    ns->m_timeout_ms = 2000;
    ns->m_max_transaction = 0;
    
    return 0;
}

static void net_dns_source_ns_fini(net_dns_source_t source) {
    net_dns_source_ns_t ns = net_dns_source_data(source);
    net_dns_manage_t manage = net_dns_source_manager(source);

    if (manage->m_debug) {
        CPE_INFO(
            manage->m_em, "dns-cli: ns %s free!",
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), source));
    }
    
    if (ns->m_address) {
        net_address_free(ns->m_address);
        ns->m_address = NULL;
    }

    if (ns->m_dgram) {
        net_dgram_free(ns->m_dgram);
        ns->m_dgram = NULL;
    }
    
    if (ns->m_tcp_cli) {
        net_dns_ns_cli_endpoint_free(ns->m_tcp_cli);
        ns->m_tcp_cli = NULL;
    }
}

void net_dns_source_ns_dump(write_stream_t ws, net_dns_source_t source) {
    net_dns_source_ns_t ns = net_dns_source_data(source);
    
    stream_printf(ws, "ns[");
    if (ns->m_address) {
        net_address_print(ws, ns->m_address);
    }
    stream_printf(ws, "]");
}

int net_dns_source_ns_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    struct net_dns_source_ns_ctx * ns_ctx = net_dns_task_ctx_data(task_ctx);
    net_dns_source_ns_t ns = net_dns_source_data(source);
    
    net_dns_task_ctx_set_retry_count(task_ctx, ns->m_retry_count);
    net_dns_task_ctx_set_timeout(task_ctx, ns->m_timeout_ms);

    ns_ctx->m_transaction = ++ns->m_max_transaction;
    
    return 0;
}

void net_dns_source_ns_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}

int net_dns_source_ns_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    net_dns_task_t task = net_dns_task_ctx_task(task_ctx);
    net_dns_manage_t manage = net_dns_task_ctx_manage(task_ctx);
    net_dns_source_ns_t ns = net_dns_source_data(source);
    struct net_dns_source_ns_ctx * ns_ctx = net_dns_task_ctx_data(task_ctx);

    mem_buffer_t buffer = &manage->m_data_buffer;
    mem_buffer_clear_data(buffer);

    size_t buf_capacity = 1500;
    char * buf = mem_buffer_alloc(buffer, buf_capacity);
    if (buf == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: build ns req: alloc fail!");
        return -1; 
    }

    char *p = buf;

    /* head */
    CPE_COPY_HTON16(p, &ns_ctx->m_transaction); p+=2;

    uint16_t flag = 0x0100;
    CPE_COPY_HTON16(p, &flag); p+=2;

    uint16_t qdcount = 1;
    CPE_COPY_HTON16(p, &qdcount); p+=2;

    uint16_t ancount = 0;
    CPE_COPY_HTON16(p, &ancount); p+=2;

    uint16_t nscount = 0;
    CPE_COPY_HTON16(p, &nscount); p+=2;
 
    uint16_t arcount = 0;
    CPE_COPY_HTON16(p, &arcount); p+=2;
    
    /* query */
    char * countp = p;  
    *(p++) = 0;

    const char * q;
    for(q = net_dns_task_hostname(task); *q != 0; q++) {
        if(*q != '.') {
            (*countp)++;
            *(p++) = *q;
        }
        else if(*countp != 0) {
            countp = p;
            *(p++) = 0;
        }
    }

    if (*countp != 0) {
        *(p++) = 0;
    }
 
    uint16_t qtype = 1;
    CPE_COPY_HTON16(p, &qtype); p+=2;
    
    uint16_t qclass = 1;
    CPE_COPY_HTON16(p, &qclass); p+=2;

    uint16_t buf_size = (uint32_t)(p - buf);

    switch(ns->m_trans_type) {
    case net_dns_trans_udp:
        return net_dns_source_ns_dgram_output(manage, ns, buf, buf_size);
    case net_dns_trans_tcp:
        return net_dns_source_ns_tcp_output(manage, ns, buf, buf_size);
    }
}

void net_dns_source_ns_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}

static void net_dns_source_ns_dgram_input(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t from)
{
    net_dns_source_ns_t ns = ctx;
    net_dns_source_t source = net_dns_source_from_data(ns);
    net_dns_manage_t manage = net_dns_source_manager(source);

    struct net_dns_ns_parser parser;

    if (net_dns_ns_parser_init(&parser, manage, source) != 0) {
        CPE_ERROR(manage->m_em, "dns-cli: udp <-- init parser fail");
        return;
    }
    
    if (manage->m_debug >= 2) {
        CPE_INFO(
            manage->m_em, "dns-cli: udp <-- %s",
            net_dns_ns_req_dump(manage, net_dns_manage_tmp_buffer(manage), data, (uint32_t)data_size));
    }

    int rv = net_dns_ns_parser_input(&parser, data, (uint32_t)data_size);
    if (rv < 0) {
        CPE_ERROR(manage->m_em, "dns-cli: udp <-- parse data fail");
    }
    else if (rv == 0) {
        CPE_ERROR(manage->m_em, "dns-cli: udp <-- not enough data, input=%d", (int)data_size);
    }
    else if (rv < data_size) {
        CPE_ERROR(manage->m_em, "dns-cli: udp <-- read part data, used=%d, input=%d", rv, (int)data_size);
    }
}

static int net_dns_source_ns_dgram_output(
    net_dns_manage_t manage, net_dns_source_ns_t ns, void const * buf, uint16_t buf_size)
{
    if (ns->m_dgram == NULL) {
        ns->m_dgram = net_dgram_create(
            ns->m_driver, NULL, net_dns_source_ns_dgram_input, ns);
        if (ns->m_dgram == NULL) {
            CPE_ERROR(
                manage->m_em, "dns-cli: ns[%s]: create dgram fail",
                net_address_dump(net_dns_manage_tmp_buffer(manage), ns->m_address));
            return -1;
        }
    }
    
    if (net_dgram_send(ns->m_dgram, ns->m_address, buf, buf_size) < 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: ns[%s]: send data fail",
            net_address_dump(net_dns_manage_tmp_buffer(manage), ns->m_address));
        return -1;
    }

    if (manage->m_debug >= 2) {
        char address_buf[128];
        snprintf(address_buf, sizeof(address_buf), "%s", net_address_dump(net_dns_manage_tmp_buffer(manage), ns->m_address));
        
        CPE_INFO(
            manage->m_em, "dns-cli: ns[%s]: udp --> %s",
            address_buf,
            net_dns_ns_req_dump(manage, net_dns_manage_tmp_buffer(manage), buf, buf_size));
    }

    return 0;
}

static int net_dns_source_ns_tcp_output(
    net_dns_manage_t manage, net_dns_source_ns_t ns, void const * data, uint16_t buf_size)
{
    if (ns->m_tcp_cli == NULL) {
        ns->m_tcp_cli = net_dns_ns_cli_endpoint_create(net_dns_source_from_data(ns), ns->m_driver);
        if (ns->m_tcp_cli == NULL) {
            CPE_ERROR(
                manage->m_em, "dns-cli: ns[%s]: create endpoint fail",
                net_address_dump(net_dns_manage_tmp_buffer(manage), ns->m_address));
            return -1;
        }
        net_endpoint_set_prepare_connect(ns->m_tcp_cli->m_endpoint, ns->m_tcp_connect, ns->m_tcp_connect_ctx);
    }

    return net_dns_ns_cli_endpoint_send(ns->m_tcp_cli, ns->m_address, data, buf_size);
}

