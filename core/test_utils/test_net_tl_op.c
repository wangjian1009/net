#include "cpe/pal/pal_strings.h"
#include "cmocka_all.h"
#include "test_memory.h"
#include "test_net_tl_op.h"

test_net_tl_op_t
test_net_tl_op_create(
    test_net_driver_t driver, int64_t delay_ms, uint32_t capacity,
    test_net_tl_op_cb_fun_t cb_fun, void * cb_ctx, test_net_tl_op_cb_fun_t cb_ctx_fini)
{
    test_net_tl_op_t op = mem_alloc(test_allocrator(), sizeof(struct test_net_tl_op) + capacity);

    op->m_driver = driver;
    op->m_execute_at_ms = driver->m_cur_time_ms + delay_ms;

    op->m_is_processing = 0;
    op->m_is_free = 0;

    op->m_cb_fun = cb_fun;
    op->m_cb_ctx = cb_ctx;
    op->m_cb_ctx_fini = cb_ctx_fini;

    bzero(op + 1, capacity);
    
    test_net_tl_op_t insert_after = NULL;
    TAILQ_FOREACH_REVERSE(insert_after, &driver->m_tl_ops, test_net_tl_op_list, m_next) {
        if (insert_after->m_execute_at_ms <= op->m_execute_at_ms) {
            TAILQ_INSERT_AFTER(&driver->m_tl_ops, insert_after, op, m_next);
            break;
        }
    }

    if (insert_after == NULL) {
        TAILQ_INSERT_HEAD(&driver->m_tl_ops, op, m_next);
    }
    
    return op;
}

void test_net_tl_op_free(test_net_tl_op_t op) {
    if (op->m_is_processing) {
        assert_true(!op->m_is_free);
        op->m_is_free = 1;
        return;
    }

    if (op->m_cb_ctx_fini) {
        op->m_cb_ctx_fini(op->m_cb_ctx, op);
    }
    
    TAILQ_REMOVE(&op->m_driver->m_tl_ops, op, m_next);
    mem_free(test_allocrator(), op);
}

void * test_net_tl_op_data(test_net_tl_op_t op) {
    return op + 1;
}

int64_t test_net_driver_run(test_net_driver_t driver, int64_t duration_ms) {
    int64_t begin_time_ms = driver->m_cur_time_ms;
    int64_t end_time_ms = begin_time_ms + duration_ms;
    test_net_tl_op_t op;
    
    while((op = TAILQ_FIRST(&driver->m_tl_ops)) != NULL) {
        if (op->m_execute_at_ms > end_time_ms) break;

        assert_true(!op->m_is_processing);
        assert_true(!op->m_is_free);
        
        driver->m_cur_time_ms = op->m_execute_at_ms;

        op->m_is_processing = 1;
        op->m_cb_fun(op->m_cb_ctx, op);
        op->m_is_processing = 0;
        test_net_tl_op_free(op);
    }
    
    return driver->m_cur_time_ms - begin_time_ms;
}

void test_net_tl_op_cb_free(void * ctx, test_net_tl_op_t op) {
    mem_free(test_allocrator(), ctx);
}
