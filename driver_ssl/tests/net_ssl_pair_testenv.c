#include "cmocka_all.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "net_endpoint.h"
#include "net_ssl_pair_testenv.h"

int net_ssl_pair_testenv_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint) {
    net_endpoint_set_driver_debug(endpoint, 2);
    return 0;
}

net_ssl_pair_testenv_t
net_ssl_pair_testenv_create() {
    net_ssl_pair_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ssl_pair_testenv));
    env->m_env = net_ssl_testenv_create();
    
    env->m_acceptor =
        net_ssl_testenv_create_svr_acceptor(
            env->m_env, "1.2.3.4:5678",
            net_ssl_pair_testenv_acceptor_on_new_endpoint, env);
    
    return env;
}

void net_ssl_pair_testenv_free(net_ssl_pair_testenv_t env) {
    if (env->m_acceptor) {
        net_acceptor_free(env->m_acceptor);
        env->m_acceptor = NULL;
    }
    
    net_ssl_testenv_free(env->m_env);

    mem_free(test_allocrator(), env);
}

