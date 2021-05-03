#ifndef NET_NDT7_TESTER_TARGET_H_INCLEDED
#define NET_NDT7_TESTER_TARGET_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ndt7_system.h"

NET_BEGIN_DECL

struct net_ndt7_tester_target_it {
    net_ndt7_tester_target_t (*next)(net_ndt7_tester_target_it_t it);
    char data[64];
};

enum net_ndt7_target_url_category {
    net_ndt7_target_url_ws_upload,
    net_ndt7_target_url_ws_download,
    net_ndt7_target_url_wss_upload,
    net_ndt7_target_url_wss_download,
};
#define net_ndt7_target_url_category_count (4)

net_ndt7_tester_target_t
net_ndt7_tester_target_create(net_ndt7_tester_t tester);

void net_ndt7_tester_target_free(net_ndt7_tester_target_t target);

const char * net_ndt7_tester_target_machine(net_ndt7_tester_target_t target);
int net_ndt7_tester_target_set_machine(net_ndt7_tester_target_t target, const char * machine);

const char * net_ndt7_tester_target_country(net_ndt7_tester_target_t target);
int net_ndt7_tester_target_set_country(net_ndt7_tester_target_t target, const char * country);

const char * net_ndt7_tester_target_city(net_ndt7_tester_target_t target);
int net_ndt7_tester_target_set_city(net_ndt7_tester_target_t target, const char * city);

cpe_url_t net_ndt7_tester_target_url(net_ndt7_tester_target_t target, net_ndt7_target_url_category_t category);
int net_ndt7_tester_target_set_url(
    net_ndt7_tester_target_t target, net_ndt7_target_url_category_t category, const char * url);

cpe_url_t net_ndt7_tester_target_select_upload_url(
    net_ndt7_tester_target_t target, net_ndt7_test_protocol_t protocol);

cpe_url_t net_ndt7_tester_target_select_download_url(
    net_ndt7_tester_target_t target, net_ndt7_test_protocol_t protocol);

const char * net_ndt7_target_url_category_str(net_ndt7_target_url_category_t category);

#define net_ndt7_tester_target_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
