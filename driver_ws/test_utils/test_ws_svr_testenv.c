#include "cpe/pal/pal_unistd.h"
#include "cmocka_all.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "test_net_endpoint.h"
#include "net_ws_endpoint.h"
#include "net_ws_protocol.h"
#include "test_ws_svr_testenv.h"
#include "test_ws_svr_mock_svr.h"

test_ws_svr_testenv_t
test_ws_svr_testenv_create(net_schedule_t schedule, test_net_driver_t driver, error_monitor_t em) {
    test_ws_svr_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct test_ws_svr_testenv));
    env->m_em = em;
    env->m_schedule = schedule;
    env->m_driver = driver;

    TAILQ_INIT(&env->m_svrs);
    
    return env;
}

void test_ws_svr_testenv_free(test_ws_svr_testenv_t env) {
    while(!TAILQ_EMPTY(&env->m_svrs)) {
        test_ws_svr_mock_svr_free(TAILQ_FIRST(&env->m_svrs));
    }

    mem_free(test_allocrator(), env);
}
