#ifndef NET_NDT7_SYSTEM_H_INCLEDED
#define NET_NDT7_SYSTEM_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_ndt7_test_type net_ndt7_test_type_t;
typedef struct net_ndt7_app_info * net_ndt7_app_info_t;
typedef enum net_ndt7_response_origin net_ndt7_response_origin_t;
typedef struct net_ndt7_response * net_ndt7_response_t;

typedef struct net_ndt7_connection_info * net_ndt7_connection_info_t;
typedef struct net_ndt7_bbr_info * net_ndt7_bbr_info_t;
typedef struct net_ndt7_tcp_info * net_ndt7_tcp_info_t;
typedef struct net_ndt7_measurement * net_ndt7_measurement_t;

typedef enum net_ndt7_test_protocol net_ndt7_test_protocol_t;
typedef enum net_ndt7_tester_state net_ndt7_tester_state_t;
typedef enum net_ndt7_tester_error net_ndt7_tester_error_t;
typedef struct net_ndt7_config * net_ndt7_config_t;
typedef struct net_ndt7_manage * net_ndt7_manage_t;
typedef struct net_ndt7_tester * net_ndt7_tester_t;
typedef struct net_ndt7_tester_target_it * net_ndt7_tester_target_it_t;
typedef struct net_ndt7_tester_target * net_ndt7_tester_target_t;
typedef enum net_ndt7_target_url_category net_ndt7_target_url_category_t;

NET_END_DECL

#endif
