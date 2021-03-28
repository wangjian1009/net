#include "cpe/pal/pal_unistd.h"
#include "cmocka_all.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "test_net_endpoint.h"
#include "net_http_svr_protocol.h"
#include "net_http_svr_testenv.h"

net_http_svr_testenv_t
net_http_svr_testenv_create(net_schedule_t schedule, test_net_driver_t driver, error_monitor_t em) {
    net_http_svr_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_http_svr_testenv));
    env->m_em = em;
    env->m_schedule = schedule;
    env->m_driver = driver;
    env->m_cli_protocol = net_http_test_protocol_create(schedule, em, "svr-test");
    env->m_protocol = net_http_svr_protocol_create(schedule, "http-svr-test");
    env->m_acceptor = NULL;
    return env;
}

void net_http_svr_testenv_free(net_http_svr_testenv_t env) {
    if (env->m_acceptor) {
        net_acceptor_free(env->m_acceptor);
        env->m_acceptor = NULL;
    }

    net_http_test_protocol_free(env->m_cli_protocol);
    net_http_svr_protocol_free(env->m_protocol);
    mem_free(test_allocrator(), env);
}

void net_http_svr_testenv_listen(net_http_svr_testenv_t env, const char * str_address) {
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    assert_true(env->m_acceptor == NULL);
    env->m_acceptor = net_acceptor_create(
        net_driver_from_data(env->m_driver),
        net_http_svr_protocol_to_protocol(env->m_protocol),
        address, 0,
        NULL, NULL);
    assert_true(env->m_acceptor);
}

net_http_req_t
net_http_svr_testenv_create_req(net_http_svr_testenv_t env, net_http_req_method_t method, const char * url) {
    assert_true(env->m_acceptor);

    net_address_t target_addr = net_acceptor_address(env->m_acceptor);
    
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
            env->m_cli_protocol, net_acceptor_address(env->m_acceptor), driver) == NULL)
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
net_http_svr_testenv_req_commit(net_http_svr_testenv_t env, net_http_req_t req) {
    return net_http_test_protocol_req_commit(env->m_cli_protocol, req);
}
