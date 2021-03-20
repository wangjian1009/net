#include "net_http_svr_processor.h"
#include "net_http_svr_request.h"
#include "net_http_svr_response.h"
#include "net_http_svr_protocol_i.h"

static void net_http_svr_processor_root_on_head_complete(void * ctx, net_http_svr_request_t request) {
}

static void net_http_svr_processor_root_on_complete(void * ctx, net_http_svr_request_t request) {
    net_http_svr_protocol_t service = ctx;
    
    net_http_svr_response_t response = net_http_svr_response_create(request);
    if (response == NULL) return;

    if (net_http_svr_response_append_code(response, 404, "Not Found") != 0) return;
    if (net_http_svr_response_append_complete(response) != 0) return;
}

net_http_svr_processor_t
net_http_svr_processor_create_root(net_http_svr_protocol_t service) {
    net_http_svr_processor_t processor = net_http_svr_processor_create(
        service, "root", service,
        /*env*/
        NULL,
        /*request*/
        0,
        NULL,
        NULL,
        net_http_svr_processor_root_on_head_complete,
        net_http_svr_processor_root_on_complete);

    return processor;
}

