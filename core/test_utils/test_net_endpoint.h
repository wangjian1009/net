#ifndef TEST_UTILS_NET_ENDPOINT_H_INCLEDED
#define TEST_UTILS_NET_ENDPOINT_H_INCLEDED
#include "net_endpoint.h"
#include "test_net_driver.h"

struct test_net_endpoint {
    TAILQ_ENTRY(test_net_endpoint) m_next;
    test_net_tl_op_t m_writing_op;
};

int test_net_endpoint_init(net_endpoint_t base_endpoint);
void test_net_endpoint_fini(net_endpoint_t base_endpoint);
int test_net_endpoint_connect(net_endpoint_t base_endpoint);
void test_net_endpoint_close(net_endpoint_t base_endpoint);
int test_net_endpoint_update(net_endpoint_t base_endpoint);
int test_net_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t is_enable);
int test_net_endpoint_get_mss(net_endpoint_t endpoint, uint32_t *mss);
void test_net_endpoint_write(net_endpoint_t base_endpoint);

/*utils.connect*/
void test_net_driver_expect_connect_to_remote_success(test_net_driver_t driver, const char * target, int64_t delay_ms);
void test_net_driver_expect_connect_to_remote_error(test_net_driver_t driver, const char * target, int64_t delay_ms);
void test_net_driver_expect_set_no_delay(uint8_t is_enable);
void test_net_driver_expect_get_mss(uint32_t mss);
void test_net_driver_expect_close();

/*utils.write*/
void test_net_driver_expect_next_endpoint_write_noop(test_net_driver_t driver, int64_t duration_ms);
void test_net_endpoint_expect_write_noop(net_endpoint_t base_endpoint, int64_t duration_ms);

/*utils.buf*/
void test_net_driver_expect_write_memory(
    net_endpoint_t base_endpoint,
    const char * data, uint32_t data_size,
    int64_t duration_ms);

void test_net_endpoint_expect_close(net_endpoint_t ep);

#define test_net_endpoint_assert_buf_memory(__ep, __buf, __data, __size)               \
    do {                                                                               \
        void * __check_data = NULL;                                                    \
        assert_return_code(                                                            \
            net_endpoint_buf_peak_with_size((__ep), (__buf), (__size), &__check_data), \
            0);                                                                        \
        assert_true(__check_data != NULL);                                             \
        assert_memory_equal(__check_data, (__data), (__size));                         \
        net_endpoint_buf_consume((__ep), (__buf), (__size));                           \
    } while (0)

#endif
