#ifndef TEST_UTILS_NET_ENDPOINT_H_INCLEDED
#define TEST_UTILS_NET_ENDPOINT_H_INCLEDED
#include "test_net_endpoint_action.h"

typedef enum test_net_endpoint_write_policy_type {
    test_net_endpoint_write_mock,
    test_net_endpoint_write_noop,
    test_net_endpoint_write_keep,
    test_net_endpoint_write_remove,
    test_net_endpoint_write_link,
    test_net_endpoint_write_error,
} test_net_endpoint_write_policy_type_t;

struct test_net_endpoint_write_policy {
    test_net_endpoint_write_policy_type_t m_type;
    union {
        struct {
            net_endpoint_buf_type_t m_buf_type;
        } m_keep;
        struct {
            test_net_endpoint_link_t m_link;
            int64_t m_write_delay_ms;
            net_endpoint_error_source_t m_error_source;
            int m_error_no;
            const char * m_error_msg;
        } m_link;
        struct {
            net_endpoint_error_source_t m_error_source;
            int m_error_no;
            char * m_error_msg;
            int64_t m_error_delay_ms;
        } m_error;
    };
};

struct test_net_endpoint {
    TAILQ_ENTRY(test_net_endpoint) m_next;
    struct test_net_endpoint_write_policy m_write_policy;
    uint8_t m_set_no_delay_mock;
    uint8_t m_get_mss_mock;
};

int test_net_endpoint_init(net_endpoint_t base_endpoint);
void test_net_endpoint_fini(net_endpoint_t base_endpoint);
int test_net_endpoint_connect(net_endpoint_t base_endpoint);
int test_net_endpoint_update(net_endpoint_t base_endpoint);
int test_net_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t is_enable);
int test_net_endpoint_get_mss(net_endpoint_t endpoint, uint32_t *mss);
void test_net_endpoint_write(net_endpoint_t base_endpoint);

void test_net_endpoint_write_policy_clear(test_net_endpoint_t base_endpoint);

net_endpoint_t test_net_endpoint_linked_other(test_net_driver_t driver, net_endpoint_t base_endpoint);

test_net_endpoint_t test_net_endpoint_cast(net_endpoint_t base_endpoint);

/*error*/
void test_net_endpoint_error(
    test_net_driver_t driver, net_endpoint_t base_endpoint,
    net_endpoint_error_source_t error_source, int error_no, const char * error_msg,
    int64_t delay_ms);

/*endpoint by id*/
void test_net_endpoint_id_expect_connect_success(
    test_net_driver_t driver, uint32_t ep_id, const char * target, int64_t delay_ms);

void test_net_endpoint_id_expect_connect_error(
    test_net_driver_t driver, uint32_t ep_id, const char * target,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg,
    int64_t delay_ms);

void test_net_endpoint_id_expect_connect_to_acceptor(
    test_net_driver_t driver, uint32_t ep_id,
    const char * target, int64_t delay_ms, int64_t write_delay_ms);

void test_net_endpoint_id_expect_connect_to_endpoint(
    test_net_driver_t driver, uint32_t ep_id,
    const char * target, net_endpoint_t endpoint, int64_t delay_ms, int64_t write_delay_ms);

/*next-endpoint*/
void test_net_next_endpoint_expect_connect_success(
    test_net_driver_t driver, const char * target, int64_t delay_ms);
void test_net_next_endpoint_expect_connect_error(
    test_net_driver_t driver, const char * target,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg,
    int64_t delay_ms);
void test_net_next_endpoint_expect_connect_to_acceptor(
    test_net_driver_t driver, const char * target, int64_t delay_ms, int64_t write_delay_ms);
void test_net_next_endpoint_expect_connect_to_endpoint(
    test_net_driver_t driver, const char * target, net_endpoint_t other, int64_t delay_ms, int64_t write_delay_ms);

/*endpoint*/
void test_net_endpoint_expect_connect_success(net_endpoint_t base_endpoint, const char * target, int64_t delay_ms);
void test_net_endpoint_expect_connect_error(
    net_endpoint_t base_endpoint, const char * target,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg,
    int64_t delay_ms);
void test_net_endpoint_expect_connect_to_acceptor(
    net_endpoint_t base_endpoint, const char * target, int64_t delay_ms, int64_t write_delay_ms);
void test_net_endpoint_expect_connect_to_endpoint(
    net_endpoint_t base_endpoint, const char * target, net_endpoint_t other, int64_t delay_ms, int64_t write_delay_ms);
void test_net_endpoint_expect_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
void test_net_endpoint_expect_get_mss(net_endpoint_t endpoint, uint32_t mss);

/*utils.write.keep*/
void test_net_next_endpoint_expect_write_keep(
    test_net_driver_t driver, net_endpoint_buf_type_t keep_buf);

void test_net_endpoint_expect_write_keep(
    net_endpoint_t base_endpoint, net_endpoint_buf_type_t keep_buf);

/*utils.write.noop*/
void test_net_next_endpoint_expect_write_noop(test_net_driver_t driver);
void test_net_endpoint_expect_write_noop(net_endpoint_t base_endpoint);

/*utils.write.noop*/
void test_net_next_endpoint_expect_write_error(
    test_net_driver_t driver,
    net_endpoint_error_source_t error_source, int err_no, const char * error_msg, int64_t delay_ms);

void test_net_endpoint_expect_write_error(
    net_endpoint_t base_endpoint,
    net_endpoint_error_source_t error_source, int err_no, const char * error_msg, int64_t delay_ms);

/*utils.read*/
int test_net_driver_read_from_other(
    net_endpoint_t base_endpoint, net_endpoint_t from, net_endpoint_buf_type_t buf_type);

/*utils.buf*/
void test_net_driver_expect_write_memory(
    net_endpoint_t base_endpoint,
    const char * data, uint32_t data_size,
    int64_t duration_ms);

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
