#include "net_address.h"
#include "net_link.h"
#include "net_endpoint_i.h"
#include "net_protocol_i.h"
#include "net_driver_i.h"
#include "net_router_i.h"

int net_endpoint_link_with_router(net_endpoint_t endpoint, net_address_t target_addr, net_router_t router) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    net_endpoint_t target = net_endpoint_create(router->m_driver, net_endpoint_outbound, router->m_protocol);
    if (target == NULL) {
        return -1;
    }

    net_address_t connect_to_address = net_address_copy(schedule, router->m_address);
    if (connect_to_address == NULL) {
        net_endpoint_free(target);
        return -1;
    }
    
    net_endpoint_set_remote_address(target, connect_to_address);
    if (net_endpoint_connect(target) != 0) {
        net_endpoint_free(target);
        return -1;
    }

    net_link_t link = net_link_create(endpoint, 0, target, 1);
    if (link == NULL) {
        net_endpoint_free(target);
        return -1;
    }

    if (target->m_protocol->m_endpoint_direct(target, target_addr) != 0) {
        net_link_free(link);
        return -1;
    }

    return 0;
}

int net_endpoint_link_direct(net_endpoint_t endpoint, net_address_t target_addr) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (schedule->m_direct_driver == NULL) {
        CPE_ERROR(schedule->m_em, "net_endpoint_link_direct: no direct driver!");
        return -1;
    }
    
    net_driver_t driver = schedule->m_direct_driver;

    net_endpoint_t target = net_endpoint_create(driver, net_endpoint_outbound, schedule->m_direct_protocol);
    if (target == NULL) {
        return -1;
    }

    net_endpoint_set_remote_address(target, target_addr);
    if (net_endpoint_connect(target) != 0) {
        target->m_address = NULL;
        net_endpoint_free(target);
        return -1;
    }

    net_link_t link = net_link_create(endpoint, 0, target, 1);
    if (link == NULL) {
        target->m_address = NULL;
        net_endpoint_free(target);
        return -1;
    }
    
    return 0;
}
    
int net_endpoint_link_auto_select(net_endpoint_t endpoint, net_address_t target_addr) {
    net_schedule_t schedule = endpoint->m_protocol->m_schedule;
    net_router_t router;

    TAILQ_FOREACH(router, &schedule->m_routers, m_next_for_schedule) {
        return net_endpoint_link_with_router(endpoint, target_addr, router);
    }

    return net_endpoint_link_direct(endpoint, target_addr);
}
