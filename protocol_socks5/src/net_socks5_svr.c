#include "net_protocol.h"
#include "net_schedule.h"
#include "net_socks5_svr_i.h"
#include "net_socks5_svr_endpoint_i.h"

static int net_socks5_svr_init(net_protocol_t protocol);
static void net_socks5_svr_fini(net_protocol_t protocol);

net_socks5_svr_t net_socks5_svr_create(net_schedule_t schedule) {
    net_protocol_t protocol =
        net_protocol_create(
            schedule, "socks5",
            sizeof(struct net_socks5_svr),
            net_socks5_svr_init,
            net_socks5_svr_fini,
            /*endpoint*/
            sizeof(struct net_socks5_svr_endpoint),
            net_socks5_svr_endpoint_init,
            net_socks5_svr_endpoint_fini,
            net_socks5_svr_endpoint_input,
            net_socks5_svr_endpoint_forward,
            NULL);
    if (protocol == NULL) return NULL;

    net_socks5_svr_t socks5_svr = net_protocol_data(protocol);
    
    return socks5_svr;
}

void net_socks5_svr_free(net_socks5_svr_t socks5_svr) {
    net_protocol_free(net_protocol_from_data(socks5_svr));
}

static int net_socks5_svr_init(net_protocol_t protocol) {
    net_schedule_t schedule = net_protocol_schedule(protocol);
    net_socks5_svr_t socks5_svr = net_protocol_data(protocol);

    mem_buffer_init(&socks5_svr->m_tmp_buffer, net_schedule_allocrator(schedule));
    
    return 0;
}

static void net_socks5_svr_fini(net_protocol_t protocol) {
    net_socks5_svr_t socks5_svr = net_protocol_data(protocol);

    mem_buffer_clear(&socks5_svr->m_tmp_buffer);
}
