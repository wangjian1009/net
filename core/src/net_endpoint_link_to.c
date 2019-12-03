#include "net_address.h"
#include "net_link.h"
#include "net_endpoint_i.h"
#include "net_protocol_i.h"
#include "net_driver_i.h"

int net_endpoint_link_direct(net_endpoint_t endpoint, net_address_t target_addr, uint8_t is_own) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (schedule->m_direct_driver == NULL) {
        CPE_ERROR(schedule->m_em, "net_endpoint_link_direct: no direct driver!");
        return -1;
    }
    
    net_driver_t driver = schedule->m_direct_driver;

    net_endpoint_t target = net_endpoint_create(driver, schedule->m_direct_protocol, NULL);
    if (target == NULL) {
        return -1;
    }

    net_endpoint_set_remote_address(target, target_addr, is_own);
    if (net_endpoint_connect(target) != 0) {
        if (is_own) target->m_remote_address = NULL;
        net_endpoint_free(target);
        return -1;
    }

    net_link_t link = net_link_create(endpoint, 0, target, 1);
    if (link == NULL) {
        if (is_own) target->m_remote_address = NULL;
        net_endpoint_free(target);
        return -1;
    }
    
    return 0;
}
