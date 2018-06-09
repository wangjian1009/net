#include "cpe/utils/stream.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_timer.h"
#include "net_dns_task_ctx.h"
#include "net_dns_source_nameserver_i.h"

static int net_dns_source_nameserver_init(net_dns_source_t source);
static void net_dns_source_nameserver_fini(net_dns_source_t source);
static void net_dns_source_nameserver_dump(write_stream_t ws, net_dns_source_t source);

static int net_dns_source_nameserver_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static void net_dns_source_nameserver_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static int net_dns_source_nameserver_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static void net_dns_source_nameserver_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

net_dns_source_nameserver_t
net_dns_source_nameserver_create(net_dns_manage_t manage, net_address_t addr, uint8_t is_own) {
    net_dns_source_t source =
        net_dns_source_create(
            manage,
            sizeof(struct net_dns_source_nameserver),
            net_dns_source_nameserver_init,
            net_dns_source_nameserver_fini,
            net_dns_source_nameserver_dump,
            0,
            net_dns_source_nameserver_ctx_init,
            net_dns_source_nameserver_ctx_fini,
            net_dns_source_nameserver_ctx_start,
            net_dns_source_nameserver_ctx_cancel);
    if (source == NULL) return NULL;

    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);

    if (is_own) {
        nameserver->m_address = addr;
    }
    else {
        nameserver->m_address = net_address_copy(manage->m_schedule, addr);
        if (nameserver->m_address == NULL) {
            CPE_ERROR(manage->m_em, "dns: nameserver: copy address fail!");
            net_dns_source_free(source);
            return NULL;
        }
    }

    if (manage->m_debug >= 2) {
        CPE_INFO(
            manage->m_em, "dns: %s created!",
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), source));
    }
    
    return nameserver;
}

void net_dns_source_nameserver_free(net_dns_source_nameserver_t nameserver) {
    net_dns_source_free(net_dns_source_from_data(nameserver));
}

int net_dns_source_nameserver_init(net_dns_source_t source) {
    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);

    nameserver->m_address = NULL;
    nameserver->m_endpoint = NULL;
    
    return 0;
}

void net_dns_source_nameserver_fini(net_dns_source_t source) {
    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);

    if (nameserver->m_address) {
        net_address_free(nameserver->m_address);
        nameserver->m_address = NULL;
    }

    if (nameserver->m_endpoint) {
        net_endpoint_free(nameserver->m_endpoint);
        nameserver->m_endpoint = NULL;
    }
}

void net_dns_source_nameserver_dump(write_stream_t ws, net_dns_source_t source) {
    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);
    
    stream_printf(ws, "nameserver[");
    if (nameserver->m_address) {
        net_address_print(ws, nameserver->m_address);
    }
    stream_printf(ws, "]");
}

int net_dns_source_nameserver_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    return 0;
}

void net_dns_source_nameserver_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}

int net_dns_source_nameserver_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    struct net_dns_source_nameserver_ctx * ctx = net_dns_task_ctx_data(task_ctx);
    net_dns_manage_t manage = net_dns_task_ctx_manage(task_ctx);
    
    return 0;
}

void net_dns_source_nameserver_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}
