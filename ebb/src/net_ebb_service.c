#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_ebb_service_i.h"
#include "net_ebb_connection_i.h"
#include "net_ebb_request_i.h"
#include "net_ebb_mount_point_i.h"
#include "net_ebb_processor_i.h"

static int net_ebb_service_init(net_protocol_t protocol);
static void net_ebb_service_fini(net_protocol_t protocol);

net_ebb_service_t
net_ebb_service_create(net_schedule_t schedule, const char * protocol_name) {
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
        CPE_ERROR(net_schedule_em(schedule), "ebb: %s: create protocol fail!", protocol_name);
        return NULL;
    }
    net_protocol_set_debug(protocol, 2);

    net_ebb_service_t service = net_protocol_data(protocol);
    
    return service;
}

void net_ebb_service_free(net_ebb_service_t service) {
    net_protocol_free(net_protocol_from_data(service));
}

net_protocol_t net_ebb_service_to_protocol(net_ebb_service_t service) {
    return net_protocol_from_data(service);
}

net_ebb_mount_point_t net_ebb_service_root(net_ebb_service_t service) {
    return service->m_root;
}

mem_buffer_t net_ebb_service_tmp_buffer(net_ebb_service_t service) {
    net_protocol_t protocol = net_ebb_service_to_protocol(service);
    return net_schedule_tmp_buffer(net_protocol_schedule(protocol));
}

static int net_ebb_service_init(net_protocol_t protocol) {
    net_ebb_service_t service = net_protocol_data(protocol);

    service->m_em = net_schedule_em(net_protocol_schedule(protocol));
    service->m_alloc = net_schedule_allocrator(net_protocol_schedule(protocol));
    service->m_cfg_connection_timeout_ms = 30 * 1000;

    TAILQ_INIT(&service->m_processors);
    service->m_root = NULL;
    TAILQ_INIT(&service->m_connections);
    service->m_request_sz = 0;
    
    TAILQ_INIT(&service->m_free_requests);
    TAILQ_INIT(&service->m_free_mount_points);

    mem_buffer_init(&service->m_search_path_buffer, NULL);

    net_ebb_processor_t dft_processor = net_ebb_processor_create_dft(service);
    if (dft_processor == NULL) {
        CPE_ERROR(service->m_em, "ebb: %s: create dft processor fail!", net_protocol_name(protocol));
        return -1;
    }
    
    const char * root_mp_name = "root";
    service->m_root = net_ebb_mount_point_create(
        service, root_mp_name, root_mp_name + strlen(root_mp_name), NULL, NULL, dft_processor);
    if (service->m_root == NULL) {
        CPE_ERROR(service->m_em, "ebb: %s: create root mount point fail!", net_protocol_name(protocol));
        net_ebb_processor_free(dft_processor);
        return -1;
    }
    
    return 0;
}

static void net_ebb_service_fini(net_protocol_t protocol) {
    net_ebb_service_t service = net_protocol_data(protocol);
    assert(TAILQ_EMPTY(&service->m_connections));

    if (service->m_root) {
        net_ebb_mount_point_free(service->m_root);
        service->m_root = NULL;
    }
    
    while(!TAILQ_EMPTY(&service->m_processors)) {
        net_ebb_processor_free(TAILQ_FIRST(&service->m_processors));
    }
    
    while(!TAILQ_EMPTY(&service->m_free_requests)) {
        net_ebb_request_real_free(TAILQ_FIRST(&service->m_free_requests));
    }
    
    mem_buffer_clear(&service->m_search_path_buffer);
}

static int net_ebb_service_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint) {
    return 0;
}
