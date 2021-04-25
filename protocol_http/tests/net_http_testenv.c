#include "cpe/pal/pal_string.h"
#include "cmocka_all.h"
#include "net_address.h"
#include "net_schedule.h"
#include "net_http_testenv.h"
#include "net_http_protocol.h"
#include "net_http_endpoint.h"
#include "net_http_test_response.h"

net_http_testenv_t
net_http_testenv_create() {
    net_http_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_http_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em, NULL);
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);

    env->m_http_protocol = net_http_test_protocol_create(env->m_schedule, env->m_em, "test");
    env->m_http_endpoint = NULL;
    env->m_svr_endpoint = NULL;
    
    mem_buffer_init(&env->m_tmp_buffer, test_allocrator());
    
    return env;
}

void net_http_testenv_free(net_http_testenv_t env) {
    mem_buffer_clear(&env->m_tmp_buffer);
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

void net_http_testenv_create_ep(net_http_testenv_t env) {
    assert_true(env->m_http_endpoint == NULL);
    env->m_http_endpoint =
        net_http_endpoint_create(
            net_driver_from_data(env->m_tdriver),
            env->m_http_protocol->m_protocol);

    net_endpoint_t ep = net_endpoint_from_protocol_data(env->m_schedule, env->m_http_endpoint);
    //test_net_endpoint_expect_write_noop(ep);
}

void net_http_testenv_create_ep_established(net_http_testenv_t env) {
    net_http_testenv_create_ep(env);

    assert_true(env->m_svr_endpoint == NULL);
    env->m_svr_endpoint =
        net_endpoint_create(
            net_driver_from_data(env->m_tdriver),
            net_schedule_noop_protocol(env->m_schedule),
            NULL);

    net_endpoint_t base_endpoint = net_endpoint_from_protocol_data(env->m_schedule, env->m_http_endpoint);

    net_address_t address = net_address_create_auto(env->m_schedule, "1.1.1.1:1");
    net_endpoint_set_remote_address(base_endpoint, address);
    net_address_free(address);

    test_net_endpoint_expect_connect_to_endpoint(base_endpoint, "1.1.1.1:1", env->m_svr_endpoint, 0, 0);
    
    assert_true(net_endpoint_connect(base_endpoint) == 0);
}

const char *
net_http_testenv_ep_recv_write(net_http_testenv_t env) {
    assert_true(env->m_http_endpoint != NULL);
    net_endpoint_t ep = net_endpoint_from_protocol_data(env->m_schedule, env->m_http_endpoint);
    
    uint32_t sz = net_endpoint_buf_size(ep, net_ep_buf_write);

    char * o_buf = mem_buffer_alloc(&env->m_tmp_buffer, sz + 1);
    assert_true(o_buf != NULL);

    if (sz > 0) {
        assert_true(net_endpoint_buf_recv(ep, net_ep_buf_write, o_buf, &sz) == 0);
    }

    o_buf[sz] = 0;
    return o_buf;
}

int net_http_testenv_ep_send_response(net_http_testenv_t env, const char * response) {
    assert_true(env->m_http_endpoint != NULL);
    net_endpoint_t ep = net_endpoint_from_protocol_data(env->m_schedule, env->m_http_endpoint);

    uint32_t sz = strlen(response);
    if (sz == 0) return 0;
    
    return net_endpoint_buf_append(ep, net_ep_buf_read, response, sz);
}

int net_http_testenv_send_response(net_http_testenv_t env, const char * data) {
    assert_true(env->m_http_endpoint);
    
    return net_endpoint_buf_append(
        net_http_endpoint_base_endpoint(env->m_http_endpoint),
        net_ep_buf_read,
        data, strlen(data));
}
