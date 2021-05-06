#include "yajl/yajl_gen.h"
#include "net_ndt7_json_i.h"
#include "net_ndt7_model_i.h"

void net_ndt7_response_to_json(yajl_gen gen, net_ndt7_response_t response) {
    yajl_gen_map_open(gen);

    yajl_gen_string2(gen, "AppInfo");
    yajl_gen_map_open(gen);

    yajl_gen_string2(gen, "ElapsedTime");
    yajl_gen_integer(gen, response->m_app_info.m_elapsed_time_ms);
    
    yajl_gen_string2(gen, "NumBytes");
    yajl_gen_double(gen, response->m_app_info.m_num_bytes);

    yajl_gen_map_close(gen);

    yajl_gen_string2(gen, "Origin");
    switch(response->m_origin) {
    case net_ndt7_response_origin_client:
        yajl_gen_string2(gen, "Client");
        break;
    }

    yajl_gen_string2(gen, "Test");
    switch(response->m_test_type) {
    case net_ndt7_test_upload:
        yajl_gen_string2(gen, "upload");
        break;
    case net_ndt7_test_download:
        yajl_gen_string2(gen, "download");
        break;
    case net_ndt7_test_download_and_upload:
        yajl_gen_string2(gen, "DownloadAndUpload");
        break;
    }
    
    yajl_gen_map_close(gen);
}

void net_ndt7_response_from_json(net_ndt7_response_t response, yajl_val val, error_monitor_t em) {
}

void net_ndt7_measurement_to_json(yajl_gen gen, net_ndt7_measurement_t measurement) {
    yajl_gen_map_open(gen);

    
    yajl_gen_map_close(gen);
}

void net_ndt7_measurement_from_json(net_ndt7_measurement_t measurement, yajl_val val, error_monitor_t em) {
}



