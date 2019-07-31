#include "net_ebb_response_i.h"

void net_ebb_response_free(net_ebb_response_t response) {
    net_ebb_request_t request = response->m_request;
    net_ebb_service_t service = net_ebb_connection_service(request->m_connection);
    
    response->m_request = (net_ebb_request_t)service;
    TAILQ_INSERT_TAIL(&service->m_free_responses, response, m_next);
}

void net_ebb_response_real_free(net_ebb_response_t response) {
    net_ebb_service_t service = (net_ebb_service_t)response->m_request;
    
    TAILQ_REMOVE(&service->m_free_responses, response, m_next);
    mem_free(service->m_alloc, response);
}

