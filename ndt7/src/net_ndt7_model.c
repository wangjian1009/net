#include "net_ndt7_model_i.h"

void net_ndt7_response_init(
    net_ndt7_response_t response, int64_t begin, int64_t cur_ms, double num_bytes,
    net_ndt7_test_type_t tester_type)
{
    response->m_app_info.m_elapsed_time_ms = cur_ms - begin;
    response->m_app_info.m_num_bytes = num_bytes;
    response->m_test_type = tester_type;
}

