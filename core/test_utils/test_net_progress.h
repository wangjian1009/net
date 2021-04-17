#ifndef TEST_UTILS_NET_PROGRESS_H_INCLEDED
#define TEST_UTILS_NET_PROGRESS_H_INCLEDED
#include "net_progress.h"
#include "test_net_driver.h"

struct test_net_progress {
    test_net_driver_t m_driver;
    TAILQ_ENTRY(test_net_progress) m_next;
    test_net_tl_op_t m_tl_op;
};

int test_net_progress_init(
    net_progress_t base_progress,
    const char * cmd, const char * argv[], net_progress_runing_mode_t mode);
void test_net_progress_fini(net_progress_t base_progress);

void test_net_progress_expect_execute_begin_success(
    test_net_driver_t driver, uint32_t ep_id, const char * cmd, net_progress_runing_mode_t mode);

void test_net_progress_expect_execute_begin_fail(
    test_net_driver_t driver, uint32_t ep_id, const char * cmd, net_progress_runing_mode_t mode);

void test_net_progress_expect_fini(test_net_driver_t driver, uint32_t ep_id);

#endif
