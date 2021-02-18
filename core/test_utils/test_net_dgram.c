#include <assert.h>
#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "test_memory.h"
#include "net_schedule.h"
#include "net_dgram.h"
#include "net_address.h"
#include "test_net_dgram.h"
#include "test_net_tl_op.h"

struct test_net_dgram_write_setup {
    struct test_net_dgram_write_policy m_write_policy;
    void * m_expect_buf;
    uint32_t m_expect_size;
};

int test_net_dgram_init(net_dgram_t base_dgram) {
    test_net_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));
    test_net_dgram_t dgram = net_dgram_data(base_dgram);

    TAILQ_INSERT_TAIL(&driver->m_dgrams, dgram, m_next);
    dgram->m_write_policy.m_type = test_net_dgram_write_mock;

    return 0;
}

void test_net_dgram_fini(net_dgram_t base_dgram) {
    test_net_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));
    test_net_dgram_t dgram = net_dgram_data(base_dgram);
    TAILQ_REMOVE(&driver->m_dgrams, dgram, m_next);
}

struct test_net_dgram_write_op_ctx {
    net_address_t m_target;
    net_address_t m_source;
    uint32_t m_data_size;
};

void test_net_dgram_write_op_cb(void * ctx, test_net_tl_op_t op) {
    test_net_driver_t driver = op->m_driver;
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));

    struct test_net_dgram_write_op_ctx * write_ctx = ctx;

    net_dgram_t target_base_dgram = test_net_driver_find_dgram_by_addr(driver, write_ctx->m_target);
    if (target_base_dgram == NULL) return;

    test_net_dgram_t target_dgram = net_dgram_data(target_base_dgram);
    
    if (net_dgram_driver_debug(target_base_dgram) >= 2) {
        CPE_INFO(
            net_schedule_em(schedule), "test: %s: << %d data",
            net_dgram_dump(net_schedule_tmp_buffer(schedule), target_base_dgram),
            write_ctx->m_data_size);
    }

    net_dgram_recv(target_base_dgram, write_ctx->m_source, write_ctx + 1, write_ctx->m_data_size);
}

void test_net_dgram_write_op_ctx_fini(void * ctx, test_net_tl_op_t op) {
    struct test_net_dgram_write_op_ctx * op_ctx = ctx;

    if (op_ctx->m_target) {
        net_address_free(op_ctx->m_target);
        op_ctx->m_target = NULL;
    }

    if (op_ctx->m_source) {
        net_address_free(op_ctx->m_source);
        op_ctx->m_source = NULL;
    }
    
    mem_free(test_allocrator(), op_ctx);
}

int test_net_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len) {
    net_schedule_t schedule = net_driver_schedule(net_dgram_driver(base_dgram));
    test_net_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));
    test_net_dgram_t dgram = net_dgram_data(base_dgram);
    uint32_t id = net_dgram_id(base_dgram);
    
    if (dgram->m_write_policy.m_type == test_net_dgram_write_mock) {
        check_expected(id);

        struct test_net_dgram_write_setup * setup = mock_type(struct test_net_dgram_write_setup *);
        assert_true(setup != NULL);

        dgram->m_write_policy = setup->m_write_policy;

        if (dgram->m_write_policy.m_type == test_net_dgram_write_mock) {
            assert_int_equal(data_len, setup->m_expect_size);
            assert_memory_equal(data, setup->m_expect_buf, setup->m_expect_size);
            return 0;
        }
    }

    switch(dgram->m_write_policy.m_type) {
    case test_net_dgram_write_mock:
        assert(0);
        return 0;
    case test_net_dgram_write_remove:
        return 0;
    case test_net_dgram_write_send:
        break;
    }

    struct test_net_dgram_write_op_ctx * op_ctx
        = mem_alloc(test_allocrator(), sizeof(struct test_net_dgram_write_op_ctx) + data_len);
    op_ctx->m_target = net_address_copy(schedule, target);
    op_ctx->m_source = net_dgram_address(base_dgram) ? net_address_copy(schedule, net_dgram_address(base_dgram)) : NULL;
    if (data_len) memcpy(op_ctx + 1, data, data_len);
    
    test_net_tl_op_t op = test_net_tl_op_create(
        driver,
        dgram->m_write_policy.m_send.m_write_delay_ms,
        0,
        test_net_dgram_write_op_cb,
        op_ctx,
        test_net_dgram_write_op_ctx_fini);
    
    return 0;
}

void test_net_dgram_expect_write_remove(net_dgram_t base_dgram) {
    test_net_driver_t driver = test_net_driver_cast(net_dgram_driver(base_dgram));
    assert_true(driver);

    struct test_net_dgram_write_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_dgram_write_setup));
    setup->m_write_policy.m_type = test_net_dgram_write_remove;
    setup->m_expect_buf = NULL;
    setup->m_expect_size = 0;
    
    expect_value(test_net_dgram_send, id, net_dgram_id(base_dgram));
    will_return(test_net_dgram_send, setup);
}

void test_net_dgram_expect_write_send(net_dgram_t base_dgram, int64_t delay_ms) {
    test_net_driver_t driver = test_net_driver_cast(net_dgram_driver(base_dgram));
    assert_true(driver);

    struct test_net_dgram_write_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_dgram_write_setup));
    setup->m_write_policy.m_type = test_net_dgram_write_send;
    setup->m_write_policy.m_send.m_write_delay_ms = delay_ms;
    setup->m_expect_buf = NULL;
    setup->m_expect_size = 0;

    expect_value(test_net_dgram_send, id, net_dgram_id(base_dgram));
    will_return(test_net_dgram_send, setup);
}

net_dgram_t
test_net_driver_find_dgram_by_addr(test_net_driver_t driver, net_address_t address) {
    test_net_dgram_t dgram;

    TAILQ_FOREACH(dgram, &driver->m_dgrams, m_next) {
        net_dgram_t base_dgram = net_dgram_from_data(dgram);
        if (net_address_cmp_opt(net_dgram_address(base_dgram), address) == 0) return base_dgram;
    }
    
    return NULL;
}
