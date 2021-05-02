#include "cpe/pal/pal_unistd.h"
#include "cmocka_all.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "test_net_endpoint.h"
#include "net_http_req.h"
#include "net_http_endpoint.h"
#include "net_http_svr_protocol.h"
#include "test_http_svr_testenv.h"
#include "test_http_svr_mock_svr.h"

test_http_svr_testenv_t
test_http_svr_testenv_create(net_schedule_t schedule, test_net_driver_t driver, error_monitor_t em) {
    test_http_svr_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct test_http_svr_testenv));
    env->m_em = em;
    env->m_schedule = schedule;
    env->m_driver = driver;
    env->m_cli_protocol = net_http_test_protocol_create(schedule, em, "svr-test");

    TAILQ_INIT(&env->m_svrs);
    
    return env;
}

void test_http_svr_testenv_free(test_http_svr_testenv_t env) {
    while(!TAILQ_EMPTY(&env->m_svrs)) {
        test_http_svr_mock_svr_free(TAILQ_FIRST(&env->m_svrs));
    }

    net_http_test_protocol_free(env->m_cli_protocol);
    mem_free(test_allocrator(), env);
}

net_http_req_t
test_http_svr_testenv_create_req(
    test_http_svr_testenv_t env, const char * svr_url, net_http_req_method_t method, const char * url)
{
    test_http_svr_mock_svr_t mock_svr = test_http_svr_mock_svr_find(env, svr_url);
    assert_true(mock_svr);

    net_address_t target_addr = net_acceptor_address(mock_svr->m_acceptor);
    
    struct mem_buffer buffer;
    mem_buffer_init(&buffer, test_allocrator());
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(&buffer);
    
    stream_printf((write_stream_t)&stream, "http://");
    net_address_print((write_stream_t)&stream, target_addr);
    stream_printf((write_stream_t)&stream, "/%s", url);
    stream_putc((write_stream_t)&stream, 0);
    
    const char * full_url = mem_buffer_make_continuous(&buffer, 0);

    net_driver_t driver = net_driver_from_data(env->m_driver);
    
    if (net_http_test_protocol_find_usable_ep(
            env->m_cli_protocol, net_acceptor_address(mock_svr->m_acceptor), driver) == NULL)
    {
        char buf[256];
        test_net_next_endpoint_expect_connect_to_acceptor(env->m_driver, net_address_to_string(buf, sizeof(buf), target_addr), 0, 0);
    }

    net_http_req_t req = net_http_test_protocol_create_req(env->m_cli_protocol, driver, method, full_url);
    assert_true(req);
    
    mem_buffer_clear(&buffer);

    return req;
}

net_http_test_response_t
test_http_svr_testenv_req_commit(test_http_svr_testenv_t env, net_http_req_t req) {
    return net_http_test_protocol_req_commit(env->m_cli_protocol, req);
}

net_endpoint_t
test_http_svr_testenv_req_svr_endpoint(test_http_svr_testenv_t env, net_http_req_t req) {
    net_http_endpoint_t http_ep = net_http_req_endpoint(req);
    return test_net_endpoint_linked_other(env->m_driver, net_http_endpoint_base_endpoint(http_ep));
}
