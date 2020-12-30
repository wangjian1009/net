#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_protocol_noop.h"

int net_protocol_noop_endpoint_input(net_endpoint_t endpoint) {
    net_endpoint_buf_clear(endpoint, net_ep_buf_read);
    return 0;
}

net_protocol_t
net_protocol_noop_create(net_schedule_t schedule) {
    return net_protocol_create(
        schedule,
        "noop",
        /*protocol*/
        0, NULL, NULL,
        /*endpoint*/
        0,
        NULL,
        NULL,
        net_protocol_noop_endpoint_input,
        NULL,
        NULL);
}
