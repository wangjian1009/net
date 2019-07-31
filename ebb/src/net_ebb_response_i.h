#ifndef NET_EBB_RESPONSE_H_I_INCLEDED
#define NET_EBB_RESPONSE_H_I_INCLEDED
#include "net_ebb_response.h"
#include "net_ebb_request_i.h"

struct net_ebb_response {
    net_ebb_request_t m_request;
    TAILQ_ENTRY(net_ebb_response) m_next;
};

void net_ebb_response_real_free(net_ebb_response_t response);

#endif
