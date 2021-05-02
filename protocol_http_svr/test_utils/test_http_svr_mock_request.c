#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_http_svr_protocol.h"
#include "net_http_svr_request.h"
#include "net_http_svr_response.h"
#include "test_http_svr_mock_request.h"
#include "test_net_tl_op.h"

static void test_http_svr_mock_response_op_cb(void * ctx, test_net_tl_op_t op);

struct test_http_svr_mock_response_op {
    char m_protocol_name[64];
    uint32_t m_request_id;
    uint8_t m_keep_alive;
    int m_code;
    char * m_code_msg;
    char * m_body;
};

test_http_svr_mock_request_policy_t
test_http_svr_request_mock_expect_response(
    test_net_driver_t driver, int code, const char * code_msg, const char * response, uint32_t delay_ms)
{
    test_http_svr_mock_request_policy_t policy =
        mem_buffer_alloc(
            &driver->m_setup_buffer, sizeof(struct test_http_svr_mock_request_policy));

    policy->m_type = test_http_svr_mock_request_type_response;
    policy->m_response.m_keep_alive = 1;
    policy->m_response.m_code = code;
    policy->m_response.m_code_msg = mem_buffer_strdup(&driver->m_setup_buffer, code_msg);
    policy->m_response.m_body = response ? mem_buffer_strdup(&driver->m_setup_buffer, response) : NULL;
    policy->m_response.m_delay_ms = delay_ms;

    return policy;
}

test_http_svr_mock_request_policy_t
test_http_svr_request_mock_expect_response_close(
    test_net_driver_t driver, int code, const char * code_msg, const char * response, uint32_t delay_ms)
{
    test_http_svr_mock_request_policy_t policy =
        mem_buffer_alloc(
            &driver->m_setup_buffer, sizeof(struct test_http_svr_mock_request_policy));

    policy->m_type = test_http_svr_mock_request_type_response;
    policy->m_response.m_keep_alive = 0;
    policy->m_response.m_code = code;
    policy->m_response.m_code_msg = mem_buffer_strdup(&driver->m_setup_buffer, code_msg);
    policy->m_response.m_body = response ? mem_buffer_strdup(&driver->m_setup_buffer, response) : NULL;
    policy->m_response.m_delay_ms = delay_ms;

    return policy;
}

void test_http_svr_request_mock_apply(
    test_net_driver_t driver, net_http_svr_request_t request, test_http_svr_mock_request_policy_t policy)
{
    net_http_svr_protocol_t svr_protocol = net_http_svr_request_protocol(request);
    net_http_svr_response_t response = NULL;
    
    switch(policy->m_type) {
    case test_http_svr_mock_request_type_response:
        if (policy->m_response.m_delay_ms == 0) {
            response = net_http_svr_response_create(request);
            net_http_svr_response_append_code(response, policy->m_response.m_code, policy->m_response.m_code_msg);
            net_http_svr_response_append_head(response, "Connection", policy->m_response.m_keep_alive ? "Keep-Alive" : "Close");
            
            if (policy->m_response.m_body) {
                net_http_svr_response_append_head(response, "Content-Type", "application/json");
                net_http_svr_response_append_body_identity_begin(response, strlen(policy->m_response.m_body));
                net_http_svr_response_append_body_identity_data(response, policy->m_response.m_body, strlen(policy->m_response.m_body));
            }
            net_http_svr_response_append_complete(response);
        } else {
            test_net_tl_op_t op = test_net_tl_op_create(
                driver, policy->m_response.m_delay_ms,
                sizeof(struct test_http_svr_mock_response_op),
                test_http_svr_mock_response_op_cb, driver, NULL);
            assert_true(op != NULL);

            struct test_http_svr_mock_response_op * response_op = test_net_tl_op_data(op);
            cpe_str_dup(
                response_op->m_protocol_name, sizeof(response_op->m_protocol_name),
                net_protocol_name(net_protocol_from_data(svr_protocol)));
            response_op->m_request_id = net_http_svr_request_id(request);
            response_op->m_keep_alive = policy->m_response.m_keep_alive;
            response_op->m_code = policy->m_response.m_code;
            response_op->m_code_msg = policy->m_response.m_code_msg;
            response_op->m_body = policy->m_response.m_body;
        }
        break;
    case test_http_svr_mock_request_type_noop:
        break;
    }
}

static void test_http_svr_mock_response_op_cb(void * ctx, test_net_tl_op_t op) {
    test_net_driver_t driver = ctx;
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));

    struct test_http_svr_mock_response_op * response_op = test_net_tl_op_data(op);

    net_protocol_t base_protocol = net_protocol_find(schedule, response_op->m_protocol_name);
    if (base_protocol == NULL) return;

    net_http_svr_protocol_t http_protocol = net_http_svr_protocol_cast(base_protocol);
    if (http_protocol == NULL) return;

    net_http_svr_request_t request = net_http_svr_request_find(http_protocol, response_op->m_request_id);
    if (request) {
        net_http_svr_response_t response = net_http_svr_response_create(request);
        net_http_svr_response_append_code(response, response_op->m_code, response_op->m_code_msg);
        net_http_svr_response_append_head(response, "Connection", response_op->m_keep_alive ? "Keep-Alive" : "Close");
        
        if (response_op->m_body) {
            net_http_svr_response_append_head(response, "Content-Type", "application/json");
            net_http_svr_response_append_body_identity_begin(response, strlen(response_op->m_body));
            net_http_svr_response_append_body_identity_data(
                response, response_op->m_body, strlen(response_op->m_body));
        }
        net_http_svr_response_append_complete(response);
    }
}
