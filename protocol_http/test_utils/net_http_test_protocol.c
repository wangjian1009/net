#include "test_memory.h"
#include "net_protocol.h"
#include "net_http_protocol.h"
#include "net_http_test_protocol.h"
#include "net_http_test_response.h"

net_http_test_protocol_t
net_http_test_protocol_create(net_schedule_t schedule, const char * name_postfix) {
    net_http_test_protocol_t test_protocol = mem_alloc(test_allocrator(), sizeof(struct net_http_test_protocol));
    test_protocol->m_protocol = net_http_protocol_create(schedule, name_postfix);
    net_protocol_set_debug(net_protocol_from_data(test_protocol->m_protocol), 2);

    TAILQ_INIT(&test_protocol->m_responses);

    return test_protocol;
}

void net_http_test_protocol_free(net_http_test_protocol_t protocol) {
    while(!TAILQ_EMPTY(&protocol->m_responses)) {
        net_http_test_response_free(TAILQ_FIRST(&protocol->m_responses));
    }

    net_http_protocol_free(protocol->m_protocol);

    mem_free(test_allocrator(), protocol);
}

net_http_test_response_t
net_http_test_req_commit(net_http_test_protocol_t protocol, net_http_req_t req) {
    net_http_test_response_t response = net_http_test_response_create(protocol, req);
    
    if (net_http_req_write_commit(req) != 0) {
        net_http_test_response_free(response);
        return NULL;
    }

    return response;
}
