#ifndef NET_NDT7_TARGET_H_INCLEDED
#define NET_NDT7_TARGET_H_INCLEDED
#include "net_ndt7_system.h"

NET_BEGIN_DECL

net_ndt7_target_t
net_ndt7_target_create(
    net_ndt7_tester_t tester,
    const char * city, const char * country);

void net_ndt7_target_free(net_ndt7_target_t target);


NET_END_DECL

#endif
