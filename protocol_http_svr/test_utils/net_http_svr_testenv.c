#include "cpe/pal/pal_unistd.h"
#include "cmocka_all.h"
#include "net_http_svr_protocol.h"
#include "net_http_svr_testenv.h"

net_http_svr_testenv_t
net_http_svr_testenv_create(net_schedule_t schedule) {
    net_http_svr_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_http_svr_testenv));
    env->m_protocol = net_http_svr_protocol_create(schedule, "http-svr-test");
    return env;
}

void net_http_svr_testenv_free(net_http_svr_testenv_t env) {
    net_http_svr_protocol_free(env->m_protocol);
    mem_free(test_allocrator(), env);
}
