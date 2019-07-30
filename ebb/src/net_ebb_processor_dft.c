#include "net_ebb_processor_i.h"

net_ebb_processor_t net_ebb_processor_create_dft(net_ebb_service_t service) {
    net_ebb_processor_t processor = net_ebb_processor_create(service, "default", service, NULL);
    return processor;
}

