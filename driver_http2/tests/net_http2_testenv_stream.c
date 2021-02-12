#include "cmocka_all.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "net_http2_testenv.h"

net_http2_stream_endpoint_t
net_http2_testenv_create_stream_ep(net_http2_testenv_t env) {
    return net_http2_stream_endpoint_create(env->m_stream_driver, env->m_stream_test_protocol);
}

net_http2_stream_endpoint_t
net_http2_testenv_create_stream_ep_cli(net_http2_testenv_t env, const char * host) {
    net_http2_stream_endpoint_t endpoint = net_http2_testenv_create_stream_ep(env);
    net_endpoint_t base_endpoint = net_http2_stream_endpoint_base_endpoint(endpoint);
    assert_true(base_endpoint != NULL);
        
    net_address_t address = net_address_create_auto(env->m_schedule, host);
    assert_true(address != NULL);
    net_endpoint_set_remote_address(base_endpoint, address);
    net_address_free(address);

    return endpoint;
}

net_acceptor_t
net_http2_testenv_create_stream_acceptor(net_http2_testenv_t env, const char * str_address) {
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_acceptor_t acceptor =
        net_acceptor_create(
            net_driver_from_data(env->m_stream_driver),
            env->m_stream_test_protocol,
            address, 0,
            NULL, NULL);

    net_address_free(address);

    return acceptor;
}

net_http2_stream_endpoint_t
net_http2_testenv_stream_ep_other(net_http2_testenv_t env, net_http2_stream_endpoint_t ep) {
    net_http2_req_t req = net_http2_stream_endpoint_req(ep);
    net_http2_endpoint_t control = net_http2_req_endpoint(req);
    if (control == NULL) return NULL;

    net_endpoint_t control_base = net_http2_endpoint_base_endpoint(control);
    assert_true(control_base);

    net_endpoint_t other_base = test_net_endpoint_linked_other(env->m_tdriver, control_base);
    if (other_base == NULL) return NULL;

    net_http2_endpoint_t other_control = net_http2_endpoint_cast(other_base);
    assert_true(other_control);

    //net_http2_stream_endpoint_find_by_stream_id(other_control, net_http2_stream_endpoint_stream_id(ep));
    return NULL;
}
