#include "cmocka_all.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/url.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_ndt7_testenv.h"
#include "net_ndt7_manage.h"
#include "net_ndt7_tester.h"
#include "net_ndt7_tester_target.h"

net_ndt7_testenv_t
net_ndt7_testenv_create() {
    net_ndt7_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ndt7_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);
    env->m_tdns = test_net_dns_create(env->m_tdriver);
    env->m_external_svr = test_http_svr_testenv_create(env->m_schedule, env->m_tdriver, env->m_em);

    env->m_ndt_manager =
        net_ndt7_manage_create(
            test_allocrator(), env->m_em, env->m_schedule, net_driver_from_data(env->m_tdriver));
    assert_true(env->m_ndt_manager);

    return env;
}

void net_ndt7_testenv_free(net_ndt7_testenv_t env) {
    test_http_svr_testenv_free(env->m_external_svr);
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
    }
}

const char * net_ndt7_testenv_dump_tester(mem_buffer_t buffer, net_ndt7_tester_t tester) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    net_ndt7_testenv_print_tester((write_stream_t)&stream, tester);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

