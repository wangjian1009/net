#include "cmocka_all.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "test_net_endpoint.h"
#include "net_ssl_cli_underline_i.h"
#include "net_ssl_svr_underline_i.h"
#include "net_ssl_pair_testenv.h"

int net_ssl_pair_testenv_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint) {
    net_endpoint_set_driver_debug(endpoint, 1);
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

net_endpoint_t net_ssl_pair_testenv_get_svr_ep(
    net_ssl_pair_testenv_t env, net_endpoint_t client_base_ep)
{
    net_ssl_cli_endpoint_t cli_ep = net_endpoint_data(client_base_ep);
    assert_true(cli_ep->m_underline);
    assert_true(net_endpoint_driver(cli_ep->m_underline) == net_driver_from_data(env->m_env->m_tdriver));

    net_endpoint_t svr_underline_base =  test_net_endpoint_linked_other(env->m_env->m_tdriver, cli_ep->m_underline);
    assert_true(svr_underline_base != NULL);

    net_ssl_svr_undline_t svr_undline = net_endpoint_protocol_data(svr_underline_base);
    assert_true(svr_undline->m_ssl_endpoint != NULL);
    
    net_endpoint_t svr_base_endpoint = net_endpoint_from_data(svr_undline->m_ssl_endpoint);

    return svr_base_endpoint;
}
