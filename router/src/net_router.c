#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_address_matcher.h"
#include "net_router_i.h"

net_router_t
net_router_create(
    net_schedule_t schedule, net_address_t address, net_protocol_t protocol, net_driver_t driver)
{
    net_router_t router;

    if (protocol->m_endpoint_direct == NULL) {
        CPE_ERROR(schedule->m_em, "core: router: protocol %s not support direct, can`t work with router!", protocol->m_name);
        return NULL;
    }
    
    router = mem_alloc(schedule->m_alloc, sizeof(struct net_router));
    if (router == NULL) {
        CPE_ERROR(schedule->m_em, "router: alloc fail!");
        return NULL;
    }

    router->m_schedule = schedule;
    router->m_address = address;
    router->m_protocol = protocol;
    router->m_driver = driver;
    router->m_matcher_white = NULL;
    router->m_matcher_black = NULL;

    TAILQ_INSERT_TAIL(&schedule->m_routers, router, m_next_for_schedule);

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "core: router [%s@%s] created",
            protocol->m_name,
            net_address_dump(&schedule->m_tmp_buffer, address));
    }
    
    return router;
}

void net_router_free(net_router_t router) {
    net_schedule_t schedule = router->m_schedule;

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "core: router [%s@%s] free",
            router->m_protocol->m_name,
            net_address_dump(&schedule->m_tmp_buffer, router->m_address));
    }

    if (router->m_matcher_white) {
        net_address_matcher_free(router->m_matcher_white);
        router->m_matcher_white = NULL;
    }
    
    if (router->m_matcher_black) {
        net_address_matcher_free(router->m_matcher_black);
        router->m_matcher_black = NULL;
    }

    if (router->m_address) {
        net_address_free(router->m_address);
        router->m_address = NULL;
    }
    
    TAILQ_REMOVE(&schedule->m_routers, router, m_next_for_schedule);
    
    mem_free(schedule->m_alloc, router);
}

net_address_matcher_t net_router_matcher_white(net_router_t router) {
    return router->m_matcher_white;
}

net_address_matcher_t net_router_matcher_white_check_create(net_router_t router) {
    if (router->m_matcher_white == NULL) {
        router->m_matcher_white = net_address_matcher_create(router->m_schedule);
    }

    return router->m_matcher_white;
}

net_address_matcher_t net_router_matcher_black(net_router_t router) {
    return router->m_matcher_black;
}

net_address_matcher_t net_router_matcher_black_check_create(net_router_t router) {
    if (router->m_matcher_black == NULL) {
        router->m_matcher_black = net_address_matcher_create(router->m_schedule);
    }

    return router->m_matcher_black;
}

int net_router_link(net_router_t router, net_endpoint_t endpoint, net_address_t target_addr) {
    net_routerh_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    net_endpoint_t target = net_endpoint_create(router->m_driver, net_endpoint_outbound, router->m_protocol);
    if (target == NULL) {
        return -1;
    }

    if (net_endpoint_set_remote_address(target, router->m_address, 0) != 0) {
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

