#include "cmocka_all.h"
#include "net_acceptor.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_http2_testenv.h"
#include "net_http2_testenv_receiver.h"

net_http2_endpoint_t
net_http2_testenv_create_basic_ep_cli(net_http2_testenv_t env, const char * str_address) {
    net_endpoint_t base_endpoint =
        net_endpoint_create(
            net_driver_from_data(env->m_tdriver),
            net_protocol_from_data(env->m_protocol),
            NULL);

    if (str_address) {
        net_address_t address = net_address_create_auto(env->m_schedule, str_address);
        assert_true(address != NULL);
        net_endpoint_set_remote_address(base_endpoint, address);
        net_address_free(address);
    }

    net_http2_endpoint_t http2_ep = net_http2_endpoint_cast(base_endpoint);

    assert_true(net_http2_endpoint_set_runing_mode(http2_ep, net_http2_endpoint_runing_mode_cli) == 0);
    
    return http2_ep;
}

int net_http2_testenv_basic_acceptor_on_new_endpoint(void * ctx, net_endpoint_t base_endpoint) {
    net_http2_endpoint_t endpoint = net_http2_endpoint_cast(base_endpoint);
    assert_true(endpoint);

    net_http2_endpoint_set_runing_mode(endpoint, net_http2_endpoint_runing_mode_svr);
    
    return 0;
}

net_acceptor_t
net_http2_testenv_create_basic_acceptor(net_http2_testenv_t env, const char * str_address) {
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_acceptor_t acceptor =
        net_acceptor_create(
            net_driver_from_data(env->m_tdriver),
            net_protocol_from_data(env->m_protocol),
            address, 0,
            net_http2_testenv_basic_acceptor_on_new_endpoint, env);

    net_address_free(address);

    return acceptor;
}

static int net_http2_testenv_req_on_recv(void * ctx, net_http2_req_t req, void const * data, uint32_t data_len) {
    net_http2_testenv_receiver_t receiver = ctx;
    mem_buffer_append(&receiver->m_buffer, data, data_len);
    return 0;
}

net_http2_testenv_receiver_t
net_http2_testenv_create_req_receiver(net_http2_testenv_t env, net_http2_req_t req) {
    net_http2_testenv_receiver_t receiver = net_http2_testenv_receiver_create(env);
    net_http2_req_set_reader(req, receiver, NULL, net_http2_testenv_req_on_recv, NULL);
    return receiver;
}
