#include "cmocka_all.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "test_net_endpoint.h"
#include "net_http2_stream_endpoint.h"
#include "net_http2_endpoint.h"
#include "net_http2_stream_pair_testenv.h"

int net_http2_stream_pair_testenv_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint) {
    net_endpoint_set_driver_debug(endpoint, 1);
    return 0;
}

net_http2_stream_pair_testenv_t
net_http2_stream_pair_testenv_create() {
    net_http2_stream_pair_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_http2_stream_pair_testenv));
    env->m_env = net_http2_testenv_create();
    
    env->m_acceptor =
        net_http2_testenv_create_acceptor(
            env->m_env, "1.2.3.4:5678",
            net_http2_stream_pair_testenv_acceptor_on_new_endpoint, env);
    
    return env;
}

void net_http2_stream_pair_testenv_free(net_http2_stream_pair_testenv_t env) {
    if (env->m_acceptor) {
        net_acceptor_free(env->m_acceptor);
        env->m_acceptor = NULL;
    }
    
    net_http2_testenv_free(env->m_env);

    mem_free(test_allocrator(), env);
}

net_endpoint_t
net_http2_stream_pair_testenv_get_svr_stream(net_http2_stream_pair_testenv_t env, net_endpoint_t client_base_ep) {
    net_http2_endpoint_t cli_ep = net_endpoint_data(client_base_ep);
    net_endpoint_t control = net_http2_stream_endpoint_control(client_base_ep);
    assert_true(control);
    assert_true(net_endpoint_driver(control) == net_driver_from_data(env->m_env->m_tdriver));

    net_endpoint_t svr_control =  test_net_endpoint_linked_other(env->m_env->m_tdriver, control);
    assert_true(svr_control != NULL);

    /* net_endpoint_t svr_stream = net_http2_endpoint_stream(svr_control); */
    //return svr_stream;

    return NULL;
}
