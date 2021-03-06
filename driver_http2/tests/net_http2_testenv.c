#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_pair.h"
#include "net_http2_testenv.h"
#include "net_http2_testenv_receiver.h"
#include "net_http2_protocol.h"
#include "net_http2_stream_driver.h"

net_http2_testenv_t net_http2_testenv_create() {
    net_http2_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_http2_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);

    const char * addition_name = "test";

    /*basic*/
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);
    env->m_protocol =
        net_http2_protocol_create(env->m_schedule, addition_name, test_allocrator(), env->m_em);
    net_protocol_set_debug(net_protocol_from_data(env->m_protocol), 2);

    /*stream*/
    env->m_stream_test_protocol = test_net_protocol_create(env->m_schedule, "test-protocol");
    
    env->m_stream_driver =
        net_http2_stream_driver_create(
            env->m_schedule, addition_name,
            net_driver_from_data(env->m_tdriver),
            test_allocrator(), env->m_em);
    net_driver_set_debug(net_driver_from_data(env->m_stream_driver), 2);
    
    TAILQ_INIT(&env->m_receivers);

    return env;
}

void net_http2_testenv_free(net_http2_testenv_t env) {
    while(!TAILQ_EMPTY(&env->m_receivers)) {
        net_http2_testenv_receiver_free(TAILQ_FIRST(&env->m_receivers));
    }
    
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}
