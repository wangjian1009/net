#include "test_ws_endpoint_receiver.h"

void test_net_ws_endpoint_install_receiver_text(net_ws_endpoint_t endpoint) {
    
}

void test_net_ws_endpoint_install_receiver_bin(net_ws_endpoint_t endpoint) {
}

void test_net_ws_endpoint_install_receivers(net_ws_endpoint_t endpoint) {
    test_net_ws_endpoint_install_receiver_text(endpoint);
    test_net_ws_endpoint_install_receiver_bin(endpoint);
}
