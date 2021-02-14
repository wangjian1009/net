#include "cpe/utils/string_utils.h"
#include "net_http2_testenv_response.h"

net_http2_res_op_result_t
net_http2_testenv_response_on_res_header(void * ctx, net_http2_req_t req) {
    net_http2_testenv_response_t response = ctx;

    response->m_state = net_http2_res_state_reading_head;

    /* if (response->m_head_count < CPE_ARRAY_SIZE(response->m_headers)) { */
    /*     response->m_headers[response->m_head_count].m_name = cpe_str_mem_dup(test_allocrator(), name); */
    /*     response->m_headers[response->m_head_count].m_value = cpe_str_mem_dup(test_allocrator(), value); */
    /*     response->m_head_count++; */
    /* } */
    
    return net_http2_res_op_success;
}

net_http2_res_op_result_t
net_http2_testenv_response_on_res_body(void * ctx, net_http2_req_t req, void * data, size_t data_size) {
    net_http2_testenv_response_t response = ctx;
    response->m_state = net_http2_res_state_reading_body;
    mem_buffer_append(&response->m_body, data, data_size);
    return net_http2_res_op_success;
}

net_http2_res_op_result_t
net_http2_testenv_response_on_res_complete(void * ctx, net_http2_req_t req, net_http2_res_result_t result) {
    net_http2_testenv_response_t response = ctx;

    response->m_state = net_http2_res_state_completed;
    response->m_result = result;
    response->m_runing_req = NULL;
    
    return net_http2_res_op_success;
}

net_http2_testenv_response_t
net_http2_testenv_response_create(net_http2_testenv_t env, net_http2_req_t req) {
    net_http2_testenv_response_t response = mem_alloc(test_allocrator(), sizeof(struct net_http2_testenv_response));

    response->m_env = env;
    response->m_req_id = net_http2_req_id(req);
    response->m_runing_req = req;
    response->m_state = net_http2_res_state_reading_head;
    response->m_result = net_http2_res_complete;

    response->m_code = 0;
    response->m_code_msg = NULL;
    response->m_head_count = 0;
    
    if (net_http2_req_set_reader(
            req,
            response,
            net_http2_testenv_response_on_res_header,
            net_http2_testenv_response_on_res_body,
            net_http2_testenv_response_on_res_complete) != 0)
    {
        mem_free(test_allocrator(), response);
        return NULL;
    }

    mem_buffer_init(&response->m_body, test_allocrator());
    
    TAILQ_INSERT_TAIL(&env->m_responses, response, m_next);
    return response;
}

void net_http2_testenv_response_free(net_http2_testenv_response_t response) {
    net_http2_testenv_t env = response->m_env;

    if (response->m_runing_req) {
        net_http2_req_clear_reader(response->m_runing_req);
        response->m_runing_req = NULL;
    }

    if (response->m_code_msg) {
        mem_free(test_allocrator(), response->m_code_msg);
        response->m_code_msg = NULL;
    }

    uint16_t i;
    for(i = 0; i < response->m_head_count; ++i) {
        mem_free(test_allocrator(), response->m_headers[i].m_name);
        mem_free(test_allocrator(), response->m_headers[i].m_value);
    }
    response->m_head_count = 0;
    
    mem_buffer_clear(&response->m_body);
    
    TAILQ_REMOVE(&env->m_responses, response, m_next);
    mem_free(test_allocrator(), response);
}
