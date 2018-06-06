#include "net_protocol.h"
#include "net_direct_endpoint_i.h"
#include "net_endpoint_i.h"

static int net_direct_init(net_protocol_t protocol) {
    return 0;
}

static void net_direct_fini(net_protocol_t protocol) {
}

static int net_direct_endpoint_init(net_endpoint_t endpoint) {
    return 0;
}

static void net_direct_endpoint_fini(net_endpoint_t endpoint) {
}

static int net_direct_endpoint_input(net_endpoint_t endpoint) {
    if (net_endpoint_fbuf_append_from_rbuf(endpoint, 0) != 0) return -1;
    if (net_endpoint_forward(endpoint) != 0) return -1;
    return 0;
}

static int net_direct_endpoint_forward(net_endpoint_t endpoint, net_endpoint_t from) {
    return net_endpoint_wbuf_append_from_other(endpoint, from, 0);    
}

net_protocol_t
net_direct_protocol_create(net_schedule_t schedule) {
    return net_protocol_create(
        schedule, "direct",
        0,
        net_direct_init,
        net_direct_fini,
        /*endpoint*/
        0,
        net_direct_endpoint_init,
        net_direct_endpoint_fini,
        net_direct_endpoint_input,
        net_direct_endpoint_forward,
        NULL);
}
