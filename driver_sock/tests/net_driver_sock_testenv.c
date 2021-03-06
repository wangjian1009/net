#include "cmocka_all.h"
#include "test_memory.h"
#include "net_schedule.h"
#include "net_ev_driver.h"
#include "net_driver_sock_testenv.h"

net_driver_sock_testenv_t
net_driver_sock_testenv_create() {
    net_driver_sock_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_driver_sock_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);

    env->m_event_base = ev_loop_new(0);
    assert_true(env->m_event_base);

    net_ev_driver_t driver_ev =
        net_ev_driver_create(env->m_schedule, env->m_event_base);
    
    env->m_driver = net_sock_driver_from_data(driver_ev);

    return env;
}

void net_driver_sock_testenv_free(net_driver_sock_testenv_t env) {
    net_sock_driver_free(env->m_driver);
    ev_loop_destroy(env->m_event_base);
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}
