#include "net_protocol.h"
#include "net_schedule.h"
#include "net_proxy_http_svr_protocol_i.h"
#include "net_proxy_http_svr_endpoint_i.h"

static int net_proxy_http_svr_protocol_init(net_protocol_t protocol);
static void net_proxy_http_svr_protocol_fini(net_protocol_t protocol);

net_proxy_http_svr_protocol_t net_proxy_http_svr_protocol_create(net_schedule_t schedule) {
    net_protocol_t protocol =
        net_protocol_create(
            schedule, "proxy-http-svr",
            sizeof(struct net_proxy_http_svr_protocol),
            net_proxy_http_svr_protocol_init,
            net_proxy_http_svr_protocol_fini,
            /*endpoint*/
            sizeof(struct net_proxy_http_svr_endpoint),
            net_proxy_http_svr_endpoint_init,
            net_proxy_http_svr_endpoint_fini,
            net_proxy_http_svr_endpoint_input,
            net_proxy_http_svr_endpoint_forward,
            NULL,
            NULL);
    if (protocol == NULL) return NULL;

    net_proxy_http_svr_protocol_t proxy_http_svr = net_protocol_data(protocol);
    return proxy_http_svr;
}

void net_proxy_http_svr_protocol_free(net_proxy_http_svr_protocol_t proxy_http_svr_protocol) {
    net_protocol_free(net_protocol_from_data(proxy_http_svr_protocol));
}

net_proxy_http_svr_protocol_t net_proxy_http_svr_protocol_find(net_schedule_t schedule) {
    net_protocol_t protocol = net_protocol_find(schedule, "proxy-http-svr");
    return protocol ? net_protocol_data(protocol) : NULL;
}

static int net_proxy_http_svr_protocol_init(net_protocol_t protocol) {
    net_schedule_t schedule = net_protocol_schedule(protocol);
    net_proxy_http_svr_protocol_t proxy_http_svr = net_protocol_data(protocol);

    mem_buffer_init(&proxy_http_svr->m_tmp_buffer, net_schedule_allocrator(schedule));
    
    return 0;
}

static void net_proxy_http_svr_protocol_fini(net_protocol_t protocol) {
    net_proxy_http_svr_protocol_t proxy_http_svr = net_protocol_data(protocol);

    mem_buffer_clear(&proxy_http_svr->m_tmp_buffer);
}
