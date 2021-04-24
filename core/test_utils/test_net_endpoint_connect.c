#include "cmocka_all.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "test_net_endpoint.h"
#include "test_net_endpoint_link.h"
#include "test_net_tl_op.h"
#include "test_net_acceptor.h"

enum test_net_endpoint_connect_setup_type {
    test_net_endpoint_connect_setup_local,
    test_net_endpoint_connect_setup_acceptor,
    test_net_endpoint_connect_setup_endpoint,
};

struct test_net_endpoint_connect_setup {
    enum test_net_endpoint_connect_setup_type m_type;
    union {
        struct {
            net_endpoint_state_t m_state;
            net_endpoint_error_source_t m_error_source;
            uint32_t m_error_no;
            char * m_error_msg;
            int m_rv;
        } m_local;
        struct {
            int64_t m_write_delay_ms;
        } m_acceptor;
        struct {
            net_endpoint_t m_endpoint;
            int64_t m_write_delay_ms;
        } m_endpoint;
    };
    uint32_t m_ep_id;
    uint32_t m_delay_ms;
    struct test_net_endpoint_connect_setup * m_delay_setup;
};

static void test_net_endpoint_connect_cb(void * ctx, test_net_tl_op_t op);

struct test_net_endpoint_connect_setup *
test_net_endpoint_connect_setup_alloc(test_net_driver_t driver) {
    struct test_net_endpoint_connect_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_connect_setup));
    bzero(setup, sizeof(*setup));
    return setup;
}

static void test_net_endpoint_connect_will_return(
    test_net_driver_t driver, uint32_t ep_id,
    net_endpoint_state_t state,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * error_msg, int rv,
    uint32_t delay_ms, struct test_net_endpoint_connect_setup * delay_setup)
{
    struct test_net_endpoint_connect_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_connect_setup));

    setup->m_type = test_net_endpoint_connect_setup_local;
    setup->m_local.m_state = state;
    setup->m_local.m_error_source = error_source;
    setup->m_local.m_error_no = error_no;
    setup->m_local.m_error_msg = error_msg ? mem_buffer_strdup(&driver->m_setup_buffer, error_msg) : NULL;
    setup->m_local.m_rv = rv;
    setup->m_ep_id = ep_id;
    setup->m_delay_ms = delay_ms;
    setup->m_delay_setup = delay_setup;

    will_return(test_net_endpoint_connect, setup);
}

static int test_net_endpoint_connect_apply_setup(
    net_endpoint_t base_endpoint, struct test_net_endpoint_connect_setup * setup)
{
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    if (setup->m_delay_ms) {
        assert_true(setup->m_delay_setup);
        if (setup->m_delay_setup->m_ep_id == 0) {
            setup->m_delay_setup->m_ep_id = net_endpoint_id(base_endpoint);
        }
        
        test_net_tl_op_t op = test_net_tl_op_create(
            driver, setup->m_delay_ms,
            0,
            test_net_endpoint_connect_cb,
            setup->m_delay_setup,
            NULL);
    }

    switch (setup->m_type) {
    case test_net_endpoint_connect_setup_local:
        net_endpoint_set_error(base_endpoint, setup->m_local.m_error_source, setup->m_local.m_error_no, setup->m_local.m_error_msg);
        if (net_endpoint_set_state(base_endpoint, setup->m_local.m_state) != 0) return -1;
        return setup->m_local.m_rv;
    case test_net_endpoint_connect_setup_acceptor: {
        net_address_t address = net_endpoint_remote_address(base_endpoint);
        assert_true(address != NULL);

        test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

        net_acceptor_t base_acceptor = test_net_driver_find_acceptor_by_addr(driver, address);
        assert_true(base_acceptor != NULL);

        if (test_net_acceptor_accept(base_acceptor, base_endpoint, setup->m_acceptor.m_write_delay_ms) != 0) return -1;
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) return -1;
        
        return 0;
    }
    case test_net_endpoint_connect_setup_endpoint: {
        test_net_endpoint_link_t link_c2s =
            test_net_endpoint_link_create(
                net_endpoint_data(base_endpoint),
                net_endpoint_data(setup->m_endpoint.m_endpoint),
                setup->m_endpoint.m_write_delay_ms);
        if (link_c2s == NULL) return -1;
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) return -1;

        test_net_endpoint_link_t link_s2c =
            test_net_endpoint_link_create(
                net_endpoint_data(setup->m_endpoint.m_endpoint),
                net_endpoint_data(base_endpoint),
                setup->m_endpoint.m_write_delay_ms);
        if (link_s2c == NULL) return -1;
        if (net_endpoint_set_state(setup->m_endpoint.m_endpoint, net_endpoint_state_established) != 0) return -1;
        
        return 0;
    }
    }
}

int test_net_endpoint_connect(net_endpoint_t base_endpoint) {
    uint32_t id = net_endpoint_id(base_endpoint);
    char remote_addr[256];
    net_address_to_string(remote_addr, sizeof(remote_addr), net_endpoint_remote_address(base_endpoint));

    check_expected(id);
    check_expected(remote_addr);

    struct test_net_endpoint_connect_setup * setup = mock_type(struct test_net_endpoint_connect_setup *);
    if (setup == NULL) return -1;

    return test_net_endpoint_connect_apply_setup(base_endpoint, setup);
}

static void test_net_endpoint_connect_cb(void * ctx, test_net_tl_op_t op) {
    struct test_net_endpoint_connect_setup * setup = ctx;

    assert_true(setup->m_ep_id != 0);

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(op->m_driver));
    net_endpoint_t base_endpoint = net_endpoint_find(schedule, setup->m_ep_id);
    if (base_endpoint) {
        if (test_net_endpoint_connect_apply_setup(base_endpoint, setup) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        }
    }
}

void test_net_endpoint_id_expect_connect_success(
    test_net_driver_t driver, uint32_t ep_id, const char * target, int64_t delay_ms)
{
    if (ep_id == 0) {
        expect_any(test_net_endpoint_connect, id);
    }
    else {
        expect_value(test_net_endpoint_connect, id, ep_id);
    }

    if (target) {
        expect_string(test_net_endpoint_connect, remote_addr, mem_buffer_strdup(&driver->m_setup_buffer, target));
    }
    else {
        expect_any(test_net_endpoint_connect, remote_addr);
    }

    if (delay_ms == 0) {
        test_net_endpoint_connect_will_return(
            driver, ep_id, net_endpoint_state_established, net_endpoint_error_source_user, 0, NULL, 0, 0, NULL);
    }
    else {
        struct test_net_endpoint_connect_setup * delay_setup = test_net_endpoint_connect_setup_alloc(driver);
        delay_setup->m_type = test_net_endpoint_connect_setup_local;
        delay_setup->m_local.m_state = net_endpoint_state_established;
        delay_setup->m_local.m_error_source = net_endpoint_error_source_none;
        delay_setup->m_local.m_error_no = 0;
        delay_setup->m_local.m_error_msg = NULL;
        delay_setup->m_local.m_rv = 0;
        delay_setup->m_ep_id = ep_id;
        
        test_net_endpoint_connect_will_return(
            driver, ep_id, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL, 0, delay_ms, delay_setup);
    }
}

void test_net_endpoint_id_expect_connect_error(
    test_net_driver_t driver, uint32_t ep_id, const char * target,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg,
    int64_t delay_ms)
{
    expect_value(test_net_endpoint_connect, id, ep_id);
    if (target) {
        expect_string(test_net_endpoint_connect, remote_addr, mem_buffer_strdup(&driver->m_setup_buffer, target));
    }
    else {
        expect_any(test_net_endpoint_connect, remote_addr);
    }

    if (delay_ms == 0) {
        test_net_endpoint_connect_will_return(
            driver, ep_id, net_endpoint_state_error, error_source, error_no, msg, -1, 0, NULL);
    }
    else {
        struct test_net_endpoint_connect_setup * delay_setup = test_net_endpoint_connect_setup_alloc(driver);
        delay_setup->m_type = test_net_endpoint_connect_setup_local;
        delay_setup->m_local.m_state = net_endpoint_state_error;
        delay_setup->m_local.m_error_source = error_source;
        delay_setup->m_local.m_error_no = error_no;
        delay_setup->m_local.m_error_msg = msg ? mem_buffer_strdup(&driver->m_setup_buffer, msg) : NULL;
        delay_setup->m_local.m_rv = 0;
        delay_setup->m_ep_id = ep_id;
        
        test_net_endpoint_connect_will_return(
            driver, ep_id, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL, 0, delay_ms, delay_setup);
    }
}

void test_net_endpoint_id_expect_connect_to_acceptor(
    test_net_driver_t driver, uint32_t ep_id,
    const char * target, int64_t delay_ms, int64_t write_delay_ms)
{
    if (ep_id) {
        expect_value(test_net_endpoint_connect, id, ep_id);
    }
    else {
        expect_any(test_net_endpoint_connect, id);
    }
    
    if (target) {
        expect_string(test_net_endpoint_connect, remote_addr, mem_buffer_strdup(&driver->m_setup_buffer, target));
    }
    else {
        expect_any(test_net_endpoint_connect, remote_addr);
    }

    if (delay_ms == 0) {
        struct test_net_endpoint_connect_setup * setup = test_net_endpoint_connect_setup_alloc(driver);
        setup->m_type = test_net_endpoint_connect_setup_acceptor;
        setup->m_acceptor.m_write_delay_ms = write_delay_ms;
        setup->m_ep_id = ep_id;
        will_return(test_net_endpoint_connect, setup);
    }
    else {
        struct test_net_endpoint_connect_setup * delay_setup = test_net_endpoint_connect_setup_alloc(driver);
        delay_setup->m_type = test_net_endpoint_connect_setup_acceptor;
        delay_setup->m_acceptor.m_write_delay_ms = write_delay_ms;
        delay_setup->m_ep_id = ep_id;
        
        test_net_endpoint_connect_will_return(
            driver, ep_id, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL, 0, delay_ms, delay_setup);
    }
}

void test_net_endpoint_id_expect_connect_to_endpoint(
    test_net_driver_t driver, uint32_t ep_id,
    const char * target, net_endpoint_t endpoint, int64_t delay_ms, int64_t write_delay_ms)
{
    expect_value(test_net_endpoint_connect, id, ep_id);
    if (target) {
        expect_string(test_net_endpoint_connect, remote_addr, mem_buffer_strdup(&driver->m_setup_buffer, target));
    }
    else {
        expect_any(test_net_endpoint_connect, remote_addr);
    }

    if (delay_ms == 0) {
        struct test_net_endpoint_connect_setup * setup = test_net_endpoint_connect_setup_alloc(driver);
        setup->m_type = test_net_endpoint_connect_setup_endpoint;
        setup->m_endpoint.m_endpoint = endpoint;
        setup->m_endpoint.m_write_delay_ms = write_delay_ms;
        setup->m_ep_id = ep_id;
        will_return(test_net_endpoint_connect, setup);
    }
    else {
        struct test_net_endpoint_connect_setup * delay_setup = test_net_endpoint_connect_setup_alloc(driver);
        bzero(delay_setup, sizeof(*delay_setup));
        delay_setup->m_type = test_net_endpoint_connect_setup_endpoint;
        delay_setup->m_endpoint.m_endpoint = endpoint;
        delay_setup->m_endpoint.m_write_delay_ms = write_delay_ms;
        delay_setup->m_ep_id = ep_id;
        
        test_net_endpoint_connect_will_return(
            driver, ep_id, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL, 0, delay_ms, delay_setup);
    }
}

void test_net_next_endpoint_expect_connect_success(test_net_driver_t driver, const char * target, int64_t delay_ms) {
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));

    test_net_endpoint_id_expect_connect_success(
        driver, net_schedule_next_endpoint_id(schedule), target, delay_ms);
}

void test_net_next_endpoint_expect_connect_error(
    test_net_driver_t driver, const char * target,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg,
    int64_t delay_ms)
{
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));

    test_net_endpoint_id_expect_connect_error(
        driver, net_schedule_next_endpoint_id(schedule),
        target, error_source, error_no, msg, delay_ms);
}

void test_net_next_endpoint_expect_connect_to_acceptor(
    test_net_driver_t driver, const char * target, int64_t delay_ms, int64_t write_delay_ms)
{
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_endpoint_id_expect_connect_to_acceptor(
        driver, net_schedule_next_endpoint_id(schedule),
        target, delay_ms, write_delay_ms);
}

void test_net_next_endpoint_expect_connect_to_endpoint(
    test_net_driver_t driver, const char * target, net_endpoint_t other, int64_t delay_ms, int64_t write_delay_ms)
{
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_endpoint_id_expect_connect_to_endpoint(
        driver, net_schedule_next_endpoint_id(schedule),
        target, other, delay_ms, write_delay_ms);
}

void test_net_endpoint_expect_connect_success(net_endpoint_t base_endpoint, const char * target, int64_t delay_ms) {
    test_net_endpoint_id_expect_connect_success(
        net_driver_data(net_endpoint_driver(base_endpoint)),
        net_endpoint_id(base_endpoint), target, delay_ms);
}

void test_net_endpoint_expect_connect_error(
    net_endpoint_t base_endpoint, const char * target,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg,
    int64_t delay_ms)
{
    test_net_endpoint_id_expect_connect_error(
        net_driver_data(net_endpoint_driver(base_endpoint)),
        net_endpoint_id(base_endpoint),
        target, error_source, error_no, msg, delay_ms);
}

void test_net_endpoint_expect_connect_to_acceptor(
    net_endpoint_t base_endpoint, const char * target, int64_t delay_ms, int64_t write_delay_ms)
{
    test_net_endpoint_id_expect_connect_to_acceptor(
        net_driver_data(net_endpoint_driver(base_endpoint)),
        net_endpoint_id(base_endpoint),
        target, delay_ms, write_delay_ms);
}

void test_net_endpoint_expect_connect_to_endpoint(
    net_endpoint_t base_endpoint, const char * target, net_endpoint_t other, int64_t delay_ms, int64_t write_delay_ms)
{
    test_net_endpoint_id_expect_connect_to_endpoint(
        net_driver_data(net_endpoint_driver(base_endpoint)),
        net_endpoint_id(base_endpoint),
        target, other, delay_ms, write_delay_ms);
}
