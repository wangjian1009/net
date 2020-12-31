#ifndef TEST_UTILS_NET_TL_EVENT_H_INCLEDED
#define TEST_UTILS_NET_TL_EVENT_H_INCLEDED
#include "test_net_driver.h"

typedef void (*test_net_tl_op_cb_fun_t)(void * ctx, test_net_tl_op_t op);

struct test_net_tl_op {
    test_net_driver_t m_driver;
    int64_t m_execute_at_ms;
    TAILQ_ENTRY(test_net_tl_op) m_next;

    uint8_t m_is_processing;
    uint8_t m_is_free;
    
    test_net_tl_op_cb_fun_t m_cb_fun;
    void * m_cb_ctx;
    test_net_tl_op_cb_fun_t m_cb_ctx_fini;
};

test_net_tl_op_t test_net_tl_op_create(
    test_net_driver_t driver, int64_t delay_ms, uint32_t capacity,
    test_net_tl_op_cb_fun_t cb_fun, void * cb_ctx, test_net_tl_op_cb_fun_t cb_ctx_fini);

void test_net_tl_op_free(test_net_tl_op_t op);

void * test_net_tl_op_data(test_net_tl_op_t op);

void test_net_tl_op_cb_free(void * ctx, test_net_tl_op_t op);

#endif
