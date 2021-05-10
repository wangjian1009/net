#include "yajl/yajl_gen.h"
#include "cpe/utils/stream_buffer.h"
#include "yajl_utils_types.h"
#include "net_ndt7_model_i.h"
#include "net_ndt7_json_i.h"

void net_ndt7_response_init(
    net_ndt7_response_t response, int64_t begin, int64_t cur_ms, double num_bytes,
    net_ndt7_test_type_t tester_type)
{
    response->m_app_info.m_elapsed_time_ms = cur_ms - begin;
    response->m_app_info.m_num_bytes = num_bytes;
    response->m_origin = net_ndt7_response_origin_client;
    response->m_test_type = tester_type;
}

const char * net_ndt7_response_dump(mem_buffer_t buffer, net_ndt7_response_t response) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);

    yajl_gen gen = yajl_gen_alloc(NULL);
    yajl_gen_config(gen, yajl_gen_beautify, 0);
    yajl_gen_config(gen, yajl_gen_print_callback, cpe_yajl_gen_print_to_stream, (write_stream_t)&stream);
    net_ndt7_response_to_json(gen, response);    
    stream_putc((write_stream_t)&stream, 0);
    yajl_gen_free(gen);

    return mem_buffer_make_continuous(buffer, 0);
}

const char * net_ndt7_measurement_dump(mem_buffer_t buffer, net_ndt7_measurement_t measurement) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);

    yajl_gen gen = yajl_gen_alloc(NULL);
    yajl_gen_config(gen, yajl_gen_beautify, 0);
    yajl_gen_config(gen, yajl_gen_print_callback, cpe_yajl_gen_print_to_stream, (write_stream_t)&stream);
    net_ndt7_measurement_to_json(gen, measurement);    
    stream_putc((write_stream_t)&stream, 0);
    yajl_gen_free(gen);
    
    return mem_buffer_make_continuous(buffer, 0);
}
