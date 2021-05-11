#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/url.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_ndt7_testenv.h"
#include "net_ndt7_manage.h"
#include "net_ndt7_tester_i.h"
#include "net_ndt7_tester_target.h"

void net_ndt7_testenv_on_speed_progress(void * ctx, net_ndt7_tester_t tester, net_ndt7_response_t response);
void net_ndt7_testenv_on_measurement_progress(void * ctx, net_ndt7_tester_t tester, net_ndt7_measurement_t response);

void net_ndt7_testenv_on_test_complete(
    void * ctx, net_ndt7_tester_t tester,
    net_endpoint_error_source_t error_source, int error_code, const char * error_msg,
    net_ndt7_response_t response, net_ndt7_test_type_t test_type);

void net_ndt7_testenv_on_all_complete(void * ctx, net_ndt7_tester_t tester);
static void net_ndt7_testenv_log_append(net_ndt7_testenv_t env, net_ndt7_testenv_log_type_t type, yajl_val args);

net_ndt7_testenv_t
net_ndt7_testenv_create() {
    net_ndt7_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ndt7_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);
    env->m_tdns = test_net_dns_create(env->m_tdriver);
    env->m_http_svr = test_http_svr_testenv_create(env->m_schedule, env->m_tdriver, env->m_em);
    env->m_ws_svr = test_ws_svr_testenv_create(env->m_schedule, env->m_tdriver, env->m_em);

    env->m_ndt_manager =
        net_ndt7_manage_create(
            test_allocrator(), env->m_em, env->m_schedule, net_driver_from_data(env->m_tdriver));
    assert_true(env->m_ndt_manager);

    env->m_ndt_tester = NULL;
    bzero(&env->m_last_response, sizeof(env->m_last_response));
    bzero(&env->m_last_measurement, sizeof(env->m_last_measurement));
    env->m_complete_error_source = net_endpoint_error_source_none;
    env->m_complete_error_code = 0;
    env->m_complete_error_msg = NULL;

    env->m_log_count = 0;
    env->m_log_capacity = 0;
    env->m_logs = NULL;
    
    return env;
}

void net_ndt7_testenv_free(net_ndt7_testenv_t env) {
    if (env->m_logs) {
        uint16_t i;
        for(i = 0; i < env->m_log_count; i++) {
            yajl_tree_free(env->m_logs[i].m_msg);
        }
        mem_free(test_allocrator(), env->m_logs);
        env->m_logs = NULL;
        env->m_log_count = 0;
        env->m_log_capacity = 0;
    }
    
    test_ws_svr_testenv_free(env->m_ws_svr);
    test_http_svr_testenv_free(env->m_http_svr);
    net_ndt7_manage_free(env->m_ndt_manager);
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

void net_ndt7_testenv_print_tester_target(write_stream_t ws, net_ndt7_tester_target_t target) {
    uint8_t count = 0;
    const char * str_val;
    
    if ((str_val = net_ndt7_tester_target_machine(target))) {
        if (count++ > 0) stream_printf(ws, ", ");
        stream_printf(ws, "machine=%s", str_val);
    }

    if ((str_val = net_ndt7_tester_target_country(target))) {
        if (count++ > 0) stream_printf(ws, ", ");
        stream_printf(ws, "country=%s", str_val);
    }

    if ((str_val = net_ndt7_tester_target_city(target))) {
        if (count++ > 0) stream_printf(ws, ", ");
        stream_printf(ws, "city=%s", str_val);
    }
    
    net_ndt7_target_url_category_t i;
    for(i = 0; i < net_ndt7_target_url_category_count; i++) {
        stream_printf(ws, "\n");
        stream_putc_count(ws, ' ', 4);
        stream_printf(ws, "%s: ", net_ndt7_target_url_category_str(i));
        cpe_url_t url = net_ndt7_tester_target_url(target, i);
        if (url) {
            cpe_url_print(ws, url, cpe_url_print_full);
        }
        else {
            stream_printf(ws, "N/A");
        }
    }
}

void net_ndt7_testenv_print_tester(write_stream_t ws, net_ndt7_tester_t tester) {
    struct net_ndt7_tester_target_it target_it;
    net_ndt7_tester_targets(tester, &target_it);
    
    net_ndt7_tester_target_t target;
    while((target = net_ndt7_tester_target_it_next(&target_it))) {
        net_ndt7_testenv_print_tester_target(ws, target);
        stream_printf(ws, "\n");
    }
}

const char * net_ndt7_testenv_dump_tester(mem_buffer_t buffer, net_ndt7_tester_t tester) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    net_ndt7_testenv_print_tester((write_stream_t)&stream, tester);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

void net_ndt7_testenv_create_tester(net_ndt7_testenv_t env) {
    assert_true(env->m_ndt_tester == NULL);

    env->m_ndt_tester = net_ndt7_tester_create(env->m_ndt_manager);
    assert_true(env->m_ndt_tester);

    net_ndt7_tester_set_cb(
        env->m_ndt_tester,
        env,
        net_ndt7_testenv_on_speed_progress,
        net_ndt7_testenv_on_measurement_progress,
        net_ndt7_testenv_on_test_complete,
        net_ndt7_testenv_on_all_complete,
        NULL);
}

void net_ndt7_testenv_download_send_bin(net_ndt7_testenv_t env, uint32_t size, int64_t delay_ms) {
    assert_true(env->m_ndt_tester);
    assert_true(env->m_ndt_tester->m_download.m_endpoint);

    net_ws_endpoint_t svr_endpoint =
        test_ws_svr_testenv_svr_endpoint(env->m_ws_svr, env->m_ndt_tester->m_download.m_endpoint);
    assert_true(svr_endpoint);

    void * msg = mem_alloc(test_allocrator(), size);
    test_ws_svr_testenv_send_bin_msg(env->m_ws_svr, svr_endpoint, msg, size, delay_ms);
    mem_free(test_allocrator(), msg);
}

void net_ndt7_testenv_download_send_text(net_ndt7_testenv_t env, const char * msg, int64_t delay_ms) {
    assert_true(env->m_ndt_tester);
    assert_true(env->m_ndt_tester->m_download.m_endpoint);

    net_ws_endpoint_t svr_endpoint =
        test_ws_svr_testenv_svr_endpoint(env->m_ws_svr, env->m_ndt_tester->m_download.m_endpoint);
    assert_true(svr_endpoint);

    test_ws_svr_testenv_send_text_msg(env->m_ws_svr, svr_endpoint, msg, delay_ms);
}

void net_ndt7_testenv_download_close(net_ndt7_testenv_t env, uint16_t status_code, const char * msg, int64_t delay_ms) {
    assert_true(env->m_ndt_tester);
    assert_true(env->m_ndt_tester->m_download.m_endpoint);

    net_ws_endpoint_t svr_endpoint =
        test_ws_svr_testenv_svr_endpoint(env->m_ws_svr, env->m_ndt_tester->m_download.m_endpoint);
    assert_true(svr_endpoint);

    test_ws_svr_testenv_close(env->m_ws_svr, svr_endpoint, status_code, msg, delay_ms);
}

void net_ndt7_testenv_on_speed_progress(void * ctx, net_ndt7_tester_t tester, net_ndt7_response_t response) {
    net_ndt7_testenv_t env = ctx;
    env->m_last_response = *response;
}

void net_ndt7_testenv_on_measurement_progress(void * ctx, net_ndt7_tester_t tester, net_ndt7_measurement_t measurement) {
    net_ndt7_testenv_t env = ctx;
    env->m_last_measurement = *measurement;
}

void net_ndt7_testenv_on_test_complete(
    void * ctx, net_ndt7_tester_t tester,
    net_endpoint_error_source_t error_source, int error_code, const char * error_msg,
    net_ndt7_response_t response, net_ndt7_test_type_t test_type)
{
    net_ndt7_testenv_t env = ctx;

    env->m_last_response = *response;
    
    env->m_complete_error_source = error_source;
    env->m_complete_error_code = error_code;
    env->m_complete_error_msg = error_msg ? mem_buffer_strdup(&env->m_tdriver->m_setup_buffer, error_msg) : NULL;
}

void net_ndt7_testenv_on_all_complete(void * ctx, net_ndt7_tester_t tester) {
}

static void net_ndt7_testenv_log_append(net_ndt7_testenv_t env, net_ndt7_testenv_log_type_t type, yajl_val args) {
    if (env->m_log_count >= env->m_log_capacity) {
        uint16_t new_capacity = env->m_log_capacity < 16 ? 16 : env->m_log_capacity * 2;
        net_ndt7_testenv_log_t new_records =
                mem_alloc(test_allocrator(), sizeof(struct net_ndt7_testenv_log) * new_capacity);

        if (env->m_logs) {
            memcpy(new_records, env->m_logs, sizeof(struct net_ndt7_testenv_log) * env->m_log_count);
            mem_free(test_allocrator(), env->m_logs);
        }

        env->m_log_capacity = new_capacity;
        env->m_logs = new_records;
    }

    env->m_logs[env->m_log_count].m_type = type;
    env->m_logs[env->m_log_count].m_msg = args;
    env->m_log_count++;
}
