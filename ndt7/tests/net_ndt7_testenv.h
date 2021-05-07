#ifndef TESTS_NET_NDT7_TESTENV_H_INCLEDED
#define TESTS_NET_NDT7_TESTENV_H_INCLEDED
#include "yajl/yajl_tree.h"
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "test_net_driver.h"
#include "test_net_dns.h"
#include "test_http_svr_testenv.h"
#include "test_ws_svr_testenv.h"
#include "net_ndt7_system.h"
#include "net_ndt7_model.h"

typedef struct net_ndt7_testenv * net_ndt7_testenv_t;
typedef enum net_ndt7_testenv_log_type net_ndt7_testenv_log_type_t;
typedef struct net_ndt7_testenv_log * net_ndt7_testenv_log_t;

enum net_ndt7_testenv_log_type {
    net_ndt7_testenv_log_speed,
    net_ndt7_testenv_log_measurement,
    net_ndt7_testenv_log_test_complete,
};

struct net_ndt7_testenv_log {
    net_ndt7_testenv_log_type_t m_type;
    yajl_val m_msg;
};

struct net_ndt7_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
    test_net_dns_t m_tdns;
    test_http_svr_testenv_t m_http_svr;
    test_ws_svr_testenv_t m_ws_svr;
    net_ndt7_manage_t m_ndt_manager;

    net_ndt7_tester_t m_ndt_tester;

    uint16_t m_log_count;
    uint16_t m_log_capacity;
    struct net_ndt7_testenv_log * m_logs;
};

net_ndt7_testenv_t net_ndt7_testenv_create();
void net_ndt7_testenv_free(net_ndt7_testenv_t env);

void net_ndt7_testenv_create_tester(net_ndt7_testenv_t env);

void net_ndt7_testenv_download_send_bin(net_ndt7_testenv_t env, uint32_t size, int64_t delay_ms);
void net_ndt7_testenv_download_send_text(net_ndt7_testenv_t env, const char * msg, int64_t delay_ms);
void net_ndt7_testenv_download_close(net_ndt7_testenv_t env, uint16_t status_code, const char * msg, int64_t delay_ms);

void net_ndt7_testenv_print_tester_target(write_stream_t ws, net_ndt7_tester_target_t target);
void net_ndt7_testenv_print_tester(write_stream_t ws, net_ndt7_tester_t tester);
const char * net_ndt7_testenv_dump_tester(mem_buffer_t buffer, net_ndt7_tester_t tester);

#endif
