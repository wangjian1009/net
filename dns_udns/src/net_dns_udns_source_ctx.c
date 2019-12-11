#include "cpe/pal/pal_strings.h"
#include "net_dns_source.h"
#include "net_dns_task_ctx.h"
#include "net_dns_udns_source_ctx_i.h"

int net_dns_udns_source_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    net_dns_udns_source_t udns = net_dns_source_data(source);
    net_dns_udns_source_ctx_t udns_ctx = (net_dns_udns_source_ctx_t)net_dns_task_ctx_data(task_ctx);

    bzero(udns_ctx, sizeof(*udns_ctx));
    
    return 0;
}

void net_dns_udns_source_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    
}

int net_dns_udns_source_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    return 0;
}

void net_dns_udns_source_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}

