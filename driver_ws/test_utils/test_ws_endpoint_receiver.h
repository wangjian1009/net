#ifndef TEST_UTILS_WS_ENDPOINT_RECEIVER_H_INCLEDED
#define TEST_UTILS_WS_ENDPOINT_RECEIVER_H_INCLEDED
#include "net_ws_endpoint.h"

/*text*/
void test_net_ws_endpoint_install_receiver_text(net_ws_endpoint_t endpoint);

/*bin*/
void test_net_ws_endpoint_install_receiver_bin(net_ws_endpoint_t endpoint);

/*both*/
void test_net_ws_endpoint_install_receivers(net_ws_endpoint_t endpoint);

#endif
