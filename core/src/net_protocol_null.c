#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_protocol_null.h"

int net_protocol_null_endpoint_input(net_endpoint_t endpoint) {
    return 0;
}

net_protocol_t
net_protocol_null_create(net_schedule_t schedule) {
    return net_protocol_create(
        schedule,
        "null",
        /*protocol*/
        0, NULL, NULL,
        /*endpoint*/
        0,
        NULL,
        NULL,
        net_protocol_null_endpoint_input,
        NULL,
        NULL,
        NULL);
}
