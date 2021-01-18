#include <assert.h>
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_cli_endpoint_i.h"
#include "net_ssl_cli_underline_i.h"

int net_ssl_cli_endpoint_init(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_underline =
        net_endpoint_create(
            driver->m_underline_driver, driver->m_underline_protocol, NULL);
    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: connect: create inner ep fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    net_ssl_cli_underline_t underline = net_endpoint_protocol_data(endpoint->m_underline);
    underline->m_ssl_endpoint = endpoint;

    return 0;
}

void net_ssl_cli_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        net_endpoint_free(endpoint->m_underline);
        endpoint->m_underline = NULL;
    }
}

int net_ssl_cli_endpoint_connect(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: connect: no underline!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    if (net_endpoint_protocol_debug(base_endpoint) > net_endpoint_protocol_debug(endpoint->m_underline)) {
        net_endpoint_set_protocol_debug(endpoint->m_underline, net_endpoint_protocol_debug(base_endpoint));
    }
    
    if (net_endpoint_driver_debug(base_endpoint) > net_endpoint_driver_debug(endpoint->m_underline)) {
        net_endpoint_set_driver_debug(endpoint->m_underline, net_endpoint_driver_debug(base_endpoint));
    }
    
    net_address_t target_addr = net_endpoint_remote_address(base_endpoint);
    if (target_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: connect: target addr not set!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    if (net_endpoint_set_remote_address(endpoint->m_underline, target_addr) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: connect: set remote address to underline fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    return net_endpoint_connect(endpoint->m_underline);
}

void net_ssl_cli_endpoint_close(net_endpoint_t base_endpoint) {
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        if (net_endpoint_is_active(endpoint->m_underline)) {
            if (net_endpoint_set_state(endpoint->m_underline, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(endpoint->m_underline, net_endpoint_state_deleting);
            }
        }
    }
}

int net_ssl_cli_endpoint_update(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: set no delay: no underline!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }
    if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) { /*有数据等待写入 */
        if (net_ssl_cli_underline_write(endpoint->m_underline, base_endpoint, net_ep_buf_write) != 0) return -1;
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
    }

    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);

    return 0;
}

int net_ssl_cli_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: set no delay: no underline!",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_set_no_delay(endpoint->m_underline, no_delay);
}

int net_ssl_cli_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: get mss: no underline",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_get_mss(endpoint->m_underline, mss);
}

