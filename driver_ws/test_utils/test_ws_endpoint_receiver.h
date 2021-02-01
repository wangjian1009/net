#ifndef TEST_UTILS_WS_ENDPOINT_RECEIVER_H_INCLEDED
#define TEST_UTILS_WS_ENDPOINT_RECEIVER_H_INCLEDED
#include "net_ws_endpoint.h"

/*text*/
void test_net_ws_endpoint_install_receiver_text(net_ws_endpoint_t endpoint);
void test_net_ws_endpoint_expect_text_msg(net_ws_endpoint_t endpoint, const char * msg);
void test_net_ws_endpoint_expect_text_msg_disable_ep(net_ws_endpoint_t endpoint, const char * msg);

/*bin*/
void test_net_ws_endpoint_install_receiver_bin(net_ws_endpoint_t endpoint);
void test_net_ws_endpoint_expect_bin_msg(net_ws_endpoint_t endpoint, const void * msg, uint32_t msg_len);

/*both*/
void test_net_ws_endpoint_install_receivers(net_ws_endpoint_t endpoint);

#endif
