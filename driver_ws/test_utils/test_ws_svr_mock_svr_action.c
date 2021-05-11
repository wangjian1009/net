#include "cmocka_all.h"
#include "test_memory.h"
#include "test_ws_svr_mock_svr_action.h"

test_ws_svr_mock_svr_action_t
test_ws_svr_mock_svr_action_create(
    test_ws_svr_mock_svr_t svr,
    uint32_t effect_count, enum test_ws_svr_mock_svr_action_trigger trigger,
    test_net_ws_endpoint_action_t action, int64_t delay_ms)
{
    test_ws_svr_mock_svr_action_t svr_action =
        mem_alloc(test_allocrator(), sizeof(struct test_ws_svr_mock_svr_action));

    svr_action->m_svr = svr;
    test_net_ws_endpoint_action_copy(svr->m_env->m_driver, &svr_action->m_action, action);
    svr_action->m_effect_count = effect_count;
    svr_action->m_trigger = trigger;
    svr_action->m_delay_ms = delay_ms;

    TAILQ_INSERT_TAIL(&svr->m_actions, svr_action, m_next);
    
    return svr_action;
}

void test_ws_svr_mock_svr_action_free(test_ws_svr_mock_svr_action_t svr_action) {
    test_ws_svr_mock_svr_t svr = svr_action->m_svr;
    
    TAILQ_REMOVE(&svr->m_actions, svr_action, m_next);
    mem_free(test_allocrator(), svr_action);
}

void test_ws_svr_mock_svr_apply_acctions(
    test_ws_svr_mock_svr_t svr, net_ws_endpoint_t endpoint, enum test_ws_svr_mock_svr_action_trigger trigger)
{
    test_ws_svr_mock_svr_action_t action, next_action;

    for(action = TAILQ_FIRST(&svr->m_actions); action; action = next_action) {
        next_action = TAILQ_NEXT(action, m_next);

        if (action->m_trigger != trigger) return;

        test_net_ws_endpoint_apply_action(svr->m_env->m_driver, endpoint, &action->m_action, action->m_delay_ms);

        if (action->m_effect_count > 0) {
            action->m_effect_count--;
            if (action->m_effect_count == 0) {
                test_ws_svr_mock_svr_action_free(action);
            }
        }
    }
}

