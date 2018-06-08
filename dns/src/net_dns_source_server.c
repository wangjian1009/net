#include "net_address.h"
#include "net_dns_source_server_i.h"

net_dns_source_server_t
net_dns_source_server_create(net_dns_manage_t manage, net_address_t addr, uint8_t is_own) {
    net_dns_source_t source =
        net_dns_source_create(
            manage,
            sizeof(struct net_dns_source_server),
            net_dns_source_server_init,
            net_dns_source_server_fini,
            net_dns_source_server_dump,
            0,
            net_dns_source_server_ctx_init,
            net_dns_source_server_ctx_fini);
    if (source == NULL) return NULL;

    net_dns_source_server_t server = net_dns_source_data(source);

    
    return server;
}

void net_dns_source_server_free(net_dns_source_server_t server) {
}

int net_dns_source_server_init(net_dns_source_t source) {
    return 0;
}

void net_dns_source_server_fini(net_dns_source_t source) {
}

void net_dns_source_server_dump(write_stream_t ws, net_dns_source_t source) {
}

int net_dns_source_server_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    return 0;
}

void net_dns_source_server_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}
