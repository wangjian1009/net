#include <assert.h>
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_ebb_service_i.h"
#include "net_ebb_connection_i.h"
#include "net_ebb_request_i.h"

static int net_ebb_service_init(net_protocol_t protocol);
static void net_ebb_service_fini(net_protocol_t protocol);

net_ebb_service_t
net_ebb_service_create(mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, const char * protocol_name) {
    net_protocol_t protocol =
        net_protocol_create(
            schedule,
            protocol_name,
            /*protocol*/
            sizeof(struct net_ebb_service),
            net_ebb_service_init,
            net_ebb_service_fini,
            /*endpoint*/
            sizeof(struct net_ebb_connection),
            net_ebb_connection_init,
            net_ebb_connection_fini,
            net_ebb_connection_input,
            NULL,
            NULL,
            NULL);
    if (protocol == NULL) {
        CPE_ERROR(em, "ebb: %s: create protocol fail!", protocol_name);
        return NULL;
    }
    net_protocol_set_debug(protocol, 2);

    net_ebb_service_t service = net_protocol_data(protocol);
    service->m_alloc = alloc;
    service->m_em = em;
    
    return service;
}

void net_ebb_service_free(net_ebb_service_t service) {
    net_protocol_free(net_protocol_from_data(service));
}

net_protocol_t net_ebb_service_to_protocol(net_ebb_service_t service) {
    return net_protocol_from_data(service);
}

mem_buffer_t net_ebb_service_tmp_buffer(net_ebb_service_t service) {
    net_protocol_t protocol = net_ebb_service_to_protocol(service);
    return net_schedule_tmp_buffer(net_protocol_schedule(protocol));
}

static int net_ebb_service_init(net_protocol_t protocol) {
    net_ebb_service_t service = net_protocol_data(protocol);

    service->m_em = NULL;
    service->m_alloc = NULL;
    service->m_cfg_connection_timeout_ms = 30 * 1000;
    TAILQ_INIT(&service->m_connections);

    TAILQ_INIT(&service->m_free_requests);
    
    return 0;
}

static void net_ebb_service_fini(net_protocol_t protocol) {
    net_ebb_service_t service = net_protocol_data(protocol);
    assert(TAILQ_EMPTY(&service->m_connections));

    while(!TAILQ_EMPTY(&service->m_free_requests)) {
        net_ebb_request_real_free(TAILQ_FIRST(&service->m_free_requests));
    }
}

static int net_ebb_service_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint) {
    return 0;
}
