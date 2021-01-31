#include "cmocka_all.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "test_net_endpoint.h"
#include "net_ws_stream_endpoint.h"
#include "net_ws_endpoint.h"
#include "net_ws_stream_pair_testenv.h"

int net_ws_stream_pair_testenv_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint) {
    net_endpoint_set_driver_debug(endpoint, 1);
    return 0;
}

net_ws_stream_pair_testenv_t
net_ws_stream_pair_testenv_create() {
    net_ws_stream_pair_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ws_stream_pair_testenv));
    env->m_env = net_ws_testenv_create();
    
    env->m_acceptor =
        net_ws_testenv_create_acceptor(
            env->m_env, "1.2.3.4:5678",
            net_ws_stream_pair_testenv_acceptor_on_new_endpoint, env);
    
    return env;
}

void net_ws_stream_pair_testenv_free(net_ws_stream_pair_testenv_t env) {
    if (env->m_acceptor) {
        net_acceptor_free(env->m_acceptor);
        env->m_acceptor = NULL;
    }
    
    net_ws_testenv_free(env->m_env);

    mem_free(test_allocrator(), env);
}

net_endpoint_t
net_ws_stream_pair_testenv_get_svr_stream(net_ws_stream_pair_testenv_t env, net_endpoint_t client_base_ep) {
    net_ws_endpoint_t cli_ep = net_endpoint_data(client_base_ep);
    net_endpoint_t underline = net_ws_stream_endpoint_underline(client_base_ep);
    assert_true(underline);
    assert_true(net_endpoint_driver(underline) == net_driver_from_data(env->m_env->m_tdriver));

    net_endpoint_t svr_underline =  test_net_endpoint_linked_other(env->m_env->m_tdriver, underline);
    assert_true(svr_underline != NULL);

    net_endpoint_t svr_stream = net_ws_endpoint_stream(svr_underline);
        
    return svr_stream;
}
