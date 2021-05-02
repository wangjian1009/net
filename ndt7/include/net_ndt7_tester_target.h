#ifndef NET_NDT7_TESTER_TARGET_H_INCLEDED
#define NET_NDT7_TESTER_TARGET_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ndt7_system.h"

NET_BEGIN_DECL

enum net_ndt7_target_url_category {
    net_ndt7_target_url_ws_uploded,
    net_ndt7_target_url_ws_download,
    net_ndt7_target_url_wss_uploded,
    net_ndt7_target_url_wss_download,
};
#define net_ndt7_target_url_category_count (4)

const char * net_ndt7_tester_target_machine(net_ndt7_tester_target_t target);
const char * net_ndt7_tester_target_city(net_ndt7_tester_target_t target);
const char * net_ndt7_tester_target_country(net_ndt7_tester_target_t target);
cpe_url_t net_ndt7_tester_target_url(net_ndt7_tester_target_t target, net_ndt7_target_url_category_t category);

NET_END_DECL

#endif
