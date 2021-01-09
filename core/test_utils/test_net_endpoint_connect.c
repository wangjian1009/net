#include "cmocka_all.h"
#include "test_memory.h"
#include "cpe/utils/string_utils.h"
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
        } m_local;
        struct {
            net_endpoint_t m_endpoint;
        } m_endpoint;
    };
    char * m_address;
};

void test_net_endpoint_connect_will_return(
    test_net_driver_t driver,
    net_endpoint_state_t state,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * error_msg)
{
    struct test_net_endpoint_connect_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_connect_setup));

    setup->m_type = test_net_endpoint_connect_setup_local;
    setup->m_local.m_state = state;
    setup->m_local.m_error_source = error_source;
    setup->m_local.m_error_no = error_no;
    setup->m_local.m_error_msg = error_msg ? mem_buffer_strdup(&driver->m_setup_buffer, error_msg) : NULL;
    setup->m_address = NULL;

    will_return(test_net_endpoint_connect, setup);
}

int test_net_endpoint_connect_apply_setup(
    net_endpoint_t base_endpoint, struct test_net_endpoint_connect_setup * setup)
{
    switch (setup->m_type) {
    case test_net_endpoint_connect_setup_local:
        net_endpoint_set_error(base_endpoint, setup->m_local.m_error_source, setup->m_local.m_error_no, setup->m_local.m_error_msg);
        return net_endpoint_set_state(base_endpoint, setup->m_local.m_state);
    case test_net_endpoint_connect_setup_acceptor: {
        net_address_t address = net_endpoint_remote_address(base_endpoint);
        assert_true(address != NULL);

        test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

        net_acceptor_t base_acceptor = test_net_driver_find_acceptor_by_addr(driver, address);
        assert_true(base_acceptor != NULL);

        return net_sock_acceptor_accept(base_acceptor, base_endpoint);
    }
    case test_net_endpoint_connect_setup_endpoint: {
        test_net_endpoint_link_t link =
            test_net_endpoint_link_create(net_endpoint_data(base_endpoint), net_endpoint_data(setup->m_endpoint.m_endpoint));
        if (link == NULL) return -1;
        return net_endpoint_set_state(base_endpoint, net_endpoint_state_established);
    }
    }
}

int test_net_endpoint_connect(net_endpoint_t base_endpoint) {
    char remote_addr[256];
    net_address_to_string(remote_addr, sizeof(remote_addr), net_endpoint_remote_address(base_endpoint));
    
    check_expected(remote_addr);

    struct test_net_endpoint_connect_setup * setup = mock_type(struct test_net_endpoint_connect_setup *);
    if (setup == NULL) return -1;

    return test_net_endpoint_connect_apply_setup(base_endpoint, setup);
}

void test_net_endpoint_connect_cb_by_remote_addr(void * ctx, test_net_tl_op_t op) {
    struct test_net_endpoint_connect_setup * setup = ctx;

    assert_true(setup->m_address != NULL);
    
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(op->m_driver));
    net_address_t address = net_address_create_auto(schedule, setup->m_address);
    net_endpoint_t base_endpoint = test_net_driver_find_endpoint_by_remote_addr(op->m_driver, address);
    net_address_free(address);

    if (base_endpoint) {
        if (test_net_endpoint_connect_apply_setup(base_endpoint, setup) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        }
    }
}

void test_net_endpoint_connect_setup_op_ctx_free(void * ctx, test_net_tl_op_t op) {
    struct test_net_endpoint_connect_setup * setup = ctx;

    if (setup->m_type == test_net_endpoint_connect_setup_local) {
        if (setup->m_local.m_error_msg) {
            mem_free(test_allocrator(), setup->m_local.m_error_msg);
        }

    } else {
    }

    if (setup->m_address) {
        mem_free(test_allocrator(), setup->m_address);
    }

    mem_free(test_allocrator(), setup);
}

void test_net_endpoint_connect_delay_process_local(
    test_net_driver_t driver, const char * remote_addr, int64_t delay_ms,
    net_endpoint_state_t state,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * error_msg)
{
    assert_true(remote_addr != NULL);

    struct test_net_endpoint_connect_setup * setup =
        mem_alloc(test_allocrator(), sizeof(struct test_net_endpoint_connect_setup));
    setup->m_type = test_net_endpoint_connect_setup_local;
    setup->m_local.m_state = state;
    setup->m_local.m_error_source = error_source;
    setup->m_local.m_error_no = error_no;
    setup->m_local.m_error_msg = error_msg ? cpe_str_mem_dup(test_allocrator(), error_msg) : NULL;

    setup->m_address = cpe_str_mem_dup(test_allocrator(), remote_addr);
    
    test_net_tl_op_t op = test_net_tl_op_create(
        driver, delay_ms,
        0,
        test_net_endpoint_connect_cb_by_remote_addr,
        setup,
        test_net_endpoint_connect_setup_op_ctx_free);
}

void test_net_driver_expect_connect_success(test_net_driver_t driver, const char * target, int64_t delay_ms) {
    expect_string(test_net_endpoint_connect, remote_addr, target);

    if (delay_ms == 0) {
        test_net_endpoint_connect_will_return(
            driver, net_endpoint_state_established, net_endpoint_error_source_user, 0, NULL);
    }
    else {
        test_net_endpoint_connect_will_return(
            driver, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL);

        test_net_endpoint_connect_delay_process_local(
            driver, target, delay_ms,
            net_endpoint_state_established, net_endpoint_error_source_user, 0, NULL);
    }
}

void test_net_driver_expect_connect_error(
    test_net_driver_t driver, const char * target,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg,
    int64_t delay_ms)
{
    expect_string(test_net_endpoint_connect, remote_addr, target);

    if (delay_ms == 0) {
        test_net_endpoint_connect_will_return(
            driver, net_endpoint_state_network_error, error_source, error_no, msg);
    }
    else {
        test_net_endpoint_connect_will_return(
            driver, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL);

        test_net_endpoint_connect_delay_process_local(
            driver, target, delay_ms,
            net_endpoint_state_network_error, error_source, error_no, msg);
    }
}

void test_net_driver_expect_connect_to_acceptor(test_net_driver_t driver, const char * target, int64_t delay_ms) {
    expect_string(test_net_endpoint_connect, remote_addr, target);

    if (delay_ms == 0) {
        struct test_net_endpoint_connect_setup * setup
            = mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_connect_setup));
        setup->m_type = test_net_endpoint_connect_setup_acceptor;
        setup->m_address = NULL;
        will_return(test_net_endpoint_connect, setup);
    }
    else {
        test_net_endpoint_connect_will_return(
            driver, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL);

        struct test_net_endpoint_connect_setup * setup =
            mem_alloc(test_allocrator(), sizeof(struct test_net_endpoint_connect_setup));
        setup->m_type = test_net_endpoint_connect_setup_acceptor;
        setup->m_address = cpe_str_mem_dup(test_allocrator(), target);

        test_net_tl_op_t op = test_net_tl_op_create(
            driver, delay_ms,
            0,
            test_net_endpoint_connect_cb_by_remote_addr,
            setup,
            test_net_endpoint_connect_setup_op_ctx_free);
    }
}

void test_net_driver_expect_connect_to_endpoint(
    test_net_driver_t driver, const char * target, net_endpoint_t endpoint, int64_t delay_ms)
{
    expect_string(test_net_endpoint_connect, remote_addr, target);

    if (delay_ms == 0) {
        struct test_net_endpoint_connect_setup * setup
            = mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_connect_setup));
        setup->m_type = test_net_endpoint_connect_setup_endpoint;
        setup->m_endpoint.m_endpoint = endpoint;
        setup->m_address = NULL;
        will_return(test_net_endpoint_connect, setup);
    }
    else {
        test_net_endpoint_connect_will_return(
            driver, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL);

        struct test_net_endpoint_connect_setup * setup =
            mem_alloc(test_allocrator(), sizeof(struct test_net_endpoint_connect_setup));
        setup->m_type = test_net_endpoint_connect_setup_endpoint;
        setup->m_endpoint.m_endpoint = endpoint;
        setup->m_address = cpe_str_mem_dup(test_allocrator(), target);

        test_net_tl_op_t op = test_net_tl_op_create(
            driver, delay_ms,
            0,
            test_net_endpoint_connect_cb_by_remote_addr,
            setup,
            test_net_endpoint_connect_setup_op_ctx_free);
    }
}
