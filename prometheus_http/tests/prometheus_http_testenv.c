#include "cpe/pal/pal_unistd.h"
#include "cmocka_all.h"
#include "net_schedule.h"
#include "test_http_svr_mock_svr.h"
#include "prometheus_manager.h"
#include "prometheus_http_testenv.h"

prometheus_http_testenv_t
prometheus_http_testenv_create() {
    prometheus_http_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct prometheus_http_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_manager = prometheus_manager_create(test_allocrator(), env->m_em);
    env->m_net_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);
    env->m_net_driver = test_net_driver_create(env->m_net_schedule, env->m_em);
    env->m_http_svr_env = test_http_svr_testenv_create(env->m_net_schedule, env->m_net_driver, env->m_em);

    test_http_svr_mock_svr_t prometheus_svr =
        test_http_svr_mock_svr_create(env->m_http_svr_env, "prometheus", "http://127.0.0.1:1234");
    assert_true(prometheus_svr);

    env->m_http_processor =
        prometheus_http_processor_create(
            prometheus_svr->m_http_protocol,
            env->m_manager,
            env->m_em, test_allocrator());
    
    return env;
}

void prometheus_http_testenv_free(prometheus_http_testenv_t env) {
    prometheus_http_processor_free(env->m_http_processor);
    prometheus_manager_free(env->m_manager);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

net_http_req_t
prometheus_http_testenv_create_req(
    prometheus_http_testenv_t env, net_http_req_method_t method, const char * url)
{
    return test_http_svr_testenv_create_req(env->m_http_svr_env, "http://127.0.0.1:1234", method, url);
    
}
