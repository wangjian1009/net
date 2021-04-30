#ifndef NET_NDT7_TESTER_H_INCLEDED
#define NET_NDT7_TESTER_H_INCLEDED
#include "net_ndt7_system.h"

NET_BEGIN_DECL

enum net_ndt7_test_type {
    net_ndt7_test_upload,
    net_ndt7_test_download,
    net_ndt7_test_download_and_upload,
};

enum net_ndt7_tester_state {
    net_ndt7_tester_state_init,
    net_ndt7_tester_state_done,
};

net_ndt7_tester_t
net_ndt7_tester_create(net_ndt7_manage_t manager, net_ndt7_test_type_t test_type);

void net_ndt7_tester_free(net_ndt7_tester_t tester);


net_ndt7_test_type_t net_ndt7_tester_type(net_ndt7_tester_t tester);

net_ndt7_tester_state_t net_ndt7_tester_state(net_ndt7_tester_t tester);

int net_ndt7_tester_start(net_ndt7_tester_t tester);

NET_END_DECL

#endif
