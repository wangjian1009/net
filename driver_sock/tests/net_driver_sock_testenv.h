#ifndef TESTS_NET_DRIVER_SOCK_TESTENV_H_INCLEDED
#define TESTS_NET_DRIVER_SOCK_TESTENV_H_INCLEDED
#include "test_error.h"
#include "net_sock_driver_i.h"

typedef struct net_driver_sock_testenv * net_driver_sock_testenv_t;

struct net_driver_sock_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    struct event_base * m_event_base;
    net_sock_driver_t m_driver;
};

net_driver_sock_testenv_t net_driver_sock_testenv_create();
void net_driver_sock_testenv_free(net_driver_sock_testenv_t env);

#endif
