#include "ndt7_runner.h"

void ndt7_runner_tester_on_complete(void * ctx, net_ndt7_tester_t tester) {
}

int ndt7_runner_start(ndt7_runner_t runner, net_ndt7_test_type_t test_type) {
    runner->m_ndt_tester = net_ndt7_tester_create(runner->m_ndt_manager, test_type);
    if (runner->m_ndt_tester == NULL) {
        CPE_ERROR(runner->m_em, "ndt: start: create tester fail!");
        return -1;
    }

    net_ndt7_tester_set_cb(
        runner->m_ndt_tester,
        runner,
        ndt7_runner_tester_on_complete,
        NULL);

    if (net_ndt7_tester_start(runner->m_ndt_tester) != 0) {
        CPE_ERROR(runner->m_em, "ndt: start: create tester fail!");
        return -1;
    }
    
    return 0;
}
