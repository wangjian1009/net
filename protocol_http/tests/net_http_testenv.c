#include "net_schedule.h"
#include "net_http_testenv.h"
#include "net_http_protocol.h"
#include "net_http_endpoint.h"
#include "net_http_testenv_response.h"

net_http_testenv_t
net_http_testenv_create() {
    net_http_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_http_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);
    env->m_tprotocol = test_net_protocol_create(env->m_schedule, "test-protocol");
    env->m_http_protocol = net_http_protocol_create(env->m_schedule, "test");
    TAILQ_INIT(&env->m_responses);
    return env;
}

void net_http_testenv_free(net_http_testenv_t env) {
    while(!TAILQ_EMPTY(&env->m_responses)) {
        net_http_testenv_response_free(TAILQ_FIRST(&env->m_responses));
    }

    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

net_http_endpoint_t
net_http_testenv_create_ep(net_http_testenv_t env) {
    return net_http_endpoint_create(
        net_driver_from_data(env->m_tdriver),
        env->m_http_protocol);
}
