#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_kcp_endpoint_i.h"

int net_kcp_endpoint_udp_output(const char *buf, int len, ikcpcb *kcp, void *user);

int net_kcp_endpoint_init(net_endpoint_t base_endpoint) {
    net_kcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_kcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;

    endpoint->m_kcp = ikcp_create(net_endpoint_id(base_endpoint), base_endpoint);
    if (endpoint->m_kcp == NULL) {
        CPE_ERROR(
            driver->m_em, "net: kcp: %s: init: create kcp pcb fail",
            net_endpoint_dump(net_kcp_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }
    endpoint->m_kcp->output = net_kcp_endpoint_udp_output;
   
    return 0;
}

void net_kcp_endpoint_fini(net_endpoint_t base_endpoint) {
    net_kcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_kcp) {
        ikcp_release(endpoint->m_kcp);
        endpoint->m_kcp = NULL;
    }
}

int net_kcp_endpoint_connect(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_kcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_kcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    /* if (net_ws_endpoint_set_runing_mode(endpoint->m_underline, net_ws_endpoint_runing_mode_cli) != 0) { */
    /*     CPE_ERROR( */
    /*         driver->m_em, "net: kcp: %s: connect: set undnline runing mode cli failed!", */
    /*         net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint)); */
    /*     return -1; */
    /* } */
    
    /* net_endpoint_t base_underline = endpoint->m_underline->m_base_endpoint; */
    /* if (net_endpoint_driver_debug(base_endpoint) > net_endpoint_protocol_debug(base_underline)) { */
    /*     net_endpoint_set_protocol_debug(base_underline, net_endpoint_driver_debug(base_endpoint)); */
    /* } */
    
    /* net_address_t target_addr = net_endpoint_remote_address(base_endpoint); */
    /* if (target_addr == NULL) { */
    /*     CPE_ERROR( */
    /*         driver->m_em, "net: kcp: %s: connect: target addr not set!", */
    /*         net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint)); */
    /*     return -1; */
    /* } */

    /* if (net_endpoint_set_remote_address(endpoint->m_underline->m_base_endpoint, target_addr) != 0) { */
    /*     CPE_ERROR( */
    /*         driver->m_em, "net: kcp: %s: connect: set remote address to underline fail", */
    /*         net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint)); */
    /*     return -1; */
    /* } */

    /* return net_endpoint_connect(base_underline); */
    return 0;
}

int net_kcp_endpoint_update(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_kcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_kcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_read_closed:
        return 0;
    case net_endpoint_state_write_closed:
        return 0;
    case net_endpoint_state_disable:
        return 0;
    case net_endpoint_state_established:
        return 0;
    case net_endpoint_state_error:
        return 0;
    default:
        return 0;
    }
}

int net_kcp_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_kcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_kcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    return 0;
}

int net_kcp_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_kcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_kcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    return 0;
}

net_kcp_endpoint_t
net_kcp_endpoint_create(net_kcp_driver_t driver, net_protocol_t protocol) {
    net_driver_t base_driver = net_driver_from_data(driver);
    
    net_endpoint_t base_endpoint = net_endpoint_create(base_driver, protocol, NULL);
    if (base_endpoint == NULL) return NULL;

    return net_endpoint_data(base_endpoint);
}

net_kcp_endpoint_t
net_kcp_endpoint_cast(net_endpoint_t base_endpoint) {
    net_driver_t driver = net_endpoint_driver(base_endpoint);
    return net_driver_endpoint_init_fun(driver) == net_kcp_endpoint_init
        ? net_endpoint_data(base_endpoint)
        : NULL;
}

net_endpoint_t
net_kcp_endpoint_base_endpoint(net_kcp_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

int net_kcp_endpoint_udp_output(const char *buf, int len, ikcpcb *kcp, void *user) {
    return 0; //TODO:
}
