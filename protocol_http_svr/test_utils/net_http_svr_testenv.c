#include "cpe/pal/pal_unistd.h"
#include "cmocka_all.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "net_http_svr_protocol.h"
#include "net_http_svr_testenv.h"

net_http_svr_testenv_t
net_http_svr_testenv_create(net_schedule_t schedule, test_net_driver_t driver) {
    net_http_svr_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_http_svr_testenv));
    env->m_schedule = schedule;
    env->m_driver = driver;
    env->m_protocol = net_http_svr_protocol_create(schedule, "http-svr-test");
    env->m_acceptor = NULL;
    return env;
}

void net_http_svr_testenv_free(net_http_svr_testenv_t env) {
    if (env->m_acceptor) {
        net_acceptor_free(env->m_acceptor);
        env->m_acceptor = NULL;
    }
    
    net_http_svr_protocol_free(env->m_protocol);
    mem_free(test_allocrator(), env);
}

void net_http_svr_testenv_listen(net_http_svr_testenv_t env, const char * str_address) {
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    assert_true(env->m_acceptor == NULL);
    env->m_acceptor = net_acceptor_create(
        net_driver_from_data(env->m_driver),
        net_http_svr_protocol_to_protocol(env->m_protocol),
        address, 0,
        NULL, NULL);
    assert_true(env->m_acceptor);
}
