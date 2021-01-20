#include "net_ebb_processor.h"
#include "net_ebb_request.h"
#include "net_ebb_response.h"
#include "net_ebb_protocol_i.h"

static void net_ebb_processor_root_on_head_complete(void * ctx, net_ebb_request_t request) {
}

static void net_ebb_processor_root_on_complete(void * ctx, net_ebb_request_t request) {
    net_ebb_protocol_t service = ctx;
    
    net_ebb_response_t response = net_ebb_response_create(request);
    if (response == NULL) return;

    if (net_ebb_response_append_code(response, 404, "Not Found") != 0) return;
    if (net_ebb_response_append_complete(response) != 0) return;
}

net_ebb_processor_t
net_ebb_processor_create_root(net_ebb_protocol_t service) {
    net_ebb_processor_t processor = net_ebb_processor_create(
        service, "root", service,
        /*env*/
        NULL,
        /*request*/
        0,
        NULL,
        NULL,
        net_ebb_processor_root_on_head_complete,
        net_ebb_processor_root_on_complete);

    return processor;
}

