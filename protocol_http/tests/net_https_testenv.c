#include "cpe/pal/pal_string.h"
#include "cmocka_all.h"
#include "net_ssl_stream_driver.h"
#include "net_http_endpoint.h"
#include "net_https_testenv.h"

net_https_testenv_t
net_https_testenv_create() {
    net_https_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_https_testenv));
    env->m_env = net_http_testenv_create();

    env->m_ssl_driver =
        net_ssl_stream_driver_create(
            env->m_env->m_schedule,
            "test",
            net_driver_from_data(env->m_env->m_tdriver),
            test_allocrator(), env->m_env->m_em);
    
    return env;
}

void net_https_testenv_free(net_https_testenv_t env) {
    net_ssl_stream_driver_free(env->m_ssl_driver);
    net_http_testenv_free(env->m_env);
    mem_free(test_allocrator(), env);
}

net_http_endpoint_t
net_https_testenv_create_ep(net_https_testenv_t env) {
    net_http_endpoint_t http_ep =
        net_http_endpoint_create(
            net_driver_from_data(env->m_ssl_driver),
            env->m_env->m_http_protocol->m_protocol);

    net_endpoint_t ep = net_endpoint_from_protocol_data(env->m_env->m_schedule, http_ep);
    //test_net_endpoint_expect_write_noop(ep);

    return http_ep;
}
