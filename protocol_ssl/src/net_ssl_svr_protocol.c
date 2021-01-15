#include "net_protocol.h"
#include "net_schedule.h"
#include "net_ssl_svr_protocol_i.h"
#include "net_ssl_svr_endpoint_i.h"

static int net_ssl_svr_protocol_init(net_protocol_t protocol);
static void net_ssl_svr_protocol_fini(net_protocol_t protocol);

net_ssl_svr_protocol_t
net_ssl_svr_protocol_create(
    net_schedule_t schedule, const char * addition_name, mem_allocrator_t alloc, error_monitor_t em) {

    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-svr-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl-svr");
    }
        
    net_protocol_t base_protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_ssl_svr_protocol),
            net_ssl_svr_protocol_init,
            net_ssl_svr_protocol_fini,
            /*endpoint*/
            sizeof(struct net_ssl_svr_endpoint),
            net_ssl_svr_endpoint_init,
            net_ssl_svr_endpoint_fini,
            net_ssl_svr_endpoint_input,
            net_ssl_svr_endpoint_on_state_change,
            NULL);
    if (base_protocol == NULL) return NULL;

    net_ssl_svr_protocol_t ssl_protocol = net_protocol_data(base_protocol);

    ssl_protocol->m_alloc = alloc;
    ssl_protocol->m_em = em;
    
    return ssl_protocol;
}

void net_ssl_svr_protocol_free(net_ssl_svr_protocol_t svr_protocol) {
    net_protocol_free(net_protocol_from_data(svr_protocol));
}

net_ssl_svr_protocol_t
net_ssl_svr_protocol_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-svr-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl-svr");
    }

    net_protocol_t protocol = net_protocol_find(schedule, name);
    return protocol ? net_protocol_data(protocol) : NULL;
}

static int net_ssl_svr_protocol_init(net_protocol_t protocol) {
    net_ssl_svr_protocol_t ssl_protocol = net_protocol_data(protocol);
    ssl_protocol->m_alloc = NULL;
    ssl_protocol->m_em = NULL;
    return 0;
}

static void net_ssl_svr_protocol_fini(net_protocol_t protocol) {
}
