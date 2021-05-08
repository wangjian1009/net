#include "net_ndt7_tester.h"
#include "net_ndt7_model.h"
#include "ndt7_runner.h"

void ndt7_runner_tester_on_speed_progress(void * ctx, net_ndt7_tester_t tester, net_ndt7_response_t response) {
    ndt7_runner_t runner = ctx;
    printf("progress: %s\n", net_ndt7_response_dump(&runner->m_data_buffer, response));
}

void ndt7_runner_tester_on_measurement_progress(void * ctx, net_ndt7_tester_t tester, net_ndt7_measurement_t measurement) {
    ndt7_runner_t runner = ctx;
    printf("progress: %s\n", net_ndt7_measurement_dump(&runner->m_data_buffer, measurement));
}

void ndt7_runner_tester_on_test_complete(
    void * ctx, net_ndt7_tester_t tester,
    net_endpoint_error_source_t error_source, int error_code, const char * error_msg,
    net_ndt7_response_t response, net_ndt7_test_type_t test_type)
{
    ndt7_runner_t runner = ctx;
    printf("complete: %s\n", net_ndt7_response_dump(&runner->m_data_buffer, response));
}

void ndt7_runner_tester_on_all_complete(void * ctx, net_ndt7_tester_t tester) {
    ndt7_runner_t runner = ctx;

    net_ndt7_tester_error_t err = net_ndt7_tester_error(tester);
    if (err != net_ndt7_tester_error_none) {
        CPE_ERROR(
            runner->m_em, "ndt: complete: error = %s(%s)",
            net_ndt7_tester_error_str(err), net_ndt7_tester_error_msg(tester));
    }
    else {
        CPE_INFO(runner->m_em, "ndt: complete: success");
    }
    
    ndt7_runner_loop_break(runner);
}

int ndt7_runner_start(ndt7_runner_t runner, net_ndt7_test_type_t test_type) {
    runner->m_ndt_tester = net_ndt7_tester_create(runner->m_ndt_manager);
    if (runner->m_ndt_tester == NULL) {
        CPE_ERROR(runner->m_em, "ndt: start: create tester fail!");
        return -1;
    }

    net_ndt7_tester_set_type(runner->m_ndt_tester, test_type);
    
    net_ndt7_tester_set_cb(
        runner->m_ndt_tester,
        runner,
        ndt7_runner_tester_on_speed_progress,
        ndt7_runner_tester_on_measurement_progress,
        ndt7_runner_tester_on_test_complete,
        ndt7_runner_tester_on_all_complete,
        NULL);

    if (net_ndt7_tester_start(runner->m_ndt_tester) != 0) {
        CPE_ERROR(runner->m_em, "ndt: start: create tester fail!");
        return -1;
    }
    
    return 0;
}
