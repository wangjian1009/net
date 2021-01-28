#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_pair_testenv.h"

net_pair_testenv_t
net_pair_testenv_create() {
    net_pair_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_pair_testenv));
    env->m_env = net_core_testenv_create();
    return env;
}

void net_pair_testenv_free(net_pair_testenv_t env) {
    net_core_testenv_free(env->m_env);
    mem_free(test_allocrator(), env);
}

void net_pair_testenv_make_pair(net_pair_testenv_t env, net_endpoint_t ep[2]) {
    assert_true(
        net_pair_endpoint_make(
            env->m_env->m_schedule,
            env->m_env->m_tprotocol, env->m_env->m_tprotocol, 
            ep) == 0);
}
