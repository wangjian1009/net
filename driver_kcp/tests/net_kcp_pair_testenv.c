#include "cmocka_all.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "net_kcp_endpoint.h"
#include "net_kcp_pair_testenv.h"

int net_kcp_pair_testenv_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint) {
    net_endpoint_set_driver_debug(endpoint, 1);
    return 0;
}

net_kcp_pair_testenv_t
net_kcp_pair_testenv_create() {
    net_kcp_pair_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_kcp_pair_testenv));
    env->m_env = net_kcp_testenv_create();
    
    env->m_acceptor =
        net_kcp_testenv_create_acceptor(
            env->m_env, "1.2.3.4:5678",
            net_kcp_pair_testenv_acceptor_on_new_endpoint, env);
    
    return env;
}

void net_kcp_pair_testenv_free(net_kcp_pair_testenv_t env) {
    if (env->m_acceptor) {
        net_acceptor_free(env->m_acceptor);
        env->m_acceptor = NULL;
    }
    
    net_kcp_testenv_free(env->m_env);

    mem_free(test_allocrator(), env);
}

net_endpoint_t
net_kcp_pair_testenv_get_svr_stream(net_kcp_pair_testenv_t env, net_endpoint_t client_base_ep) {
    /* net_kcp_endpoint_t cli_ep = net_endpoint_data(client_base_ep); */
        
    /* return svr_stream; */
    return NULL;
}
