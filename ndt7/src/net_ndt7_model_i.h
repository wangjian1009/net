#ifndef NET_NDT7_MODEL_I_H_INCLEDED
#define NET_NDT7_MODEL_I_H_INCLEDED
#include "net_ndt7_model.h"
#include "net_ndt7_manage_i.h"

NET_BEGIN_DECL

void net_ndt7_response_init(
    net_ndt7_response_t response, int64_t begin, int64_t cur_ms, double num_bytes,
    net_ndt7_test_type_t tester_type);

NET_END_DECL

#endif
