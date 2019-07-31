#include "net_ebb_processor_i.h"

static void net_ebb_processor_dft_on_head_complete(void * ctx, net_ebb_request_t request) {
}

static void net_ebb_processor_dft_on_complete(void * ctx, net_ebb_request_t request) {
}

net_ebb_processor_t net_ebb_processor_create_dft(net_ebb_service_t service) {
    net_ebb_processor_t processor = net_ebb_processor_create(
        service, "default", service, 
        /*env*/
        NULL,
        /*request*/
        0,
        NULL,
        NULL,
        net_ebb_processor_dft_on_head_complete,
        net_ebb_processor_dft_on_complete);
    
    return processor;
}

