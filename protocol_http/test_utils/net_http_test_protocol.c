#include "test_memory.h"
#include "cpe/utils/url.h"
#include "net_protocol.h"
#include "net_http_protocol.h"
#include "net_http_endpoint.h"
#include "net_http_test_protocol.h"
#include "net_http_test_conn.h"
#include "net_http_test_response.h"

net_http_test_protocol_t
net_http_test_protocol_create(net_schedule_t schedule, error_monitor_t em, const char * name_postfix) {
    net_http_test_protocol_t test_protocol = mem_alloc(test_allocrator(), sizeof(struct net_http_test_protocol));

    test_protocol->m_em = em;

    test_protocol->m_protocol = net_http_protocol_create(schedule, name_postfix);
    net_protocol_set_debug(net_protocol_from_data(test_protocol->m_protocol), 2);

    TAILQ_INIT(&test_protocol->m_conns);
    TAILQ_INIT(&test_protocol->m_responses);

    return test_protocol;
}

void net_http_test_protocol_free(net_http_test_protocol_t protocol) {
    while(!TAILQ_EMPTY(&protocol->m_conns)) {
        net_http_test_conn_free(TAILQ_FIRST(&protocol->m_conns));
    }
    
    while(!TAILQ_EMPTY(&protocol->m_responses)) {
        net_http_test_response_free(TAILQ_FIRST(&protocol->m_responses));
    }

    net_http_protocol_free(protocol->m_protocol);

    mem_free(test_allocrator(), protocol);
}

net_http_endpoint_t
net_http_test_protocol_create_ep(net_http_test_protocol_t protocol, net_driver_t driver) {
    return net_http_endpoint_create(driver, protocol->m_protocol);
}

net_http_req_t
net_http_test_protocol_create_req(
    net_http_test_protocol_t protocol, net_http_req_method_t method, const char * str_url)
{
    cpe_url_t url = cpe_url_parse(test_allocrator(), protocol->m_em, str_url);

    /* assert_true(env->m_http_endpoint == NULL); */
    /* env->m_http_endpoint = */
    /*     net_http_endpoint_create(net_driver_from_data(env->m_env->m_tdriver)), */
    /*     net_protocol_from_data(env->m_http_protocol->m_protocol); */

    return NULL;
}

net_http_test_response_t
net_http_test_protocol_req_commit(net_http_test_protocol_t protocol, net_http_req_t req) {
    net_http_test_response_t response = net_http_test_response_create(protocol, req);
    
    if (net_http_req_write_commit(req) != 0) {
        net_http_test_response_free(response);
        return NULL;
    }

    return response;
}
