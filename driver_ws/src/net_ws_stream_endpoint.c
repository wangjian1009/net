#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ws_stream_endpoint_i.h"
#include "net_ws_endpoint_i.h"

int net_ws_stream_endpoint_create_underline(net_endpoint_t base_endpoint);
void net_ws_stream_endpoint_update_readable(net_endpoint_t base_endpoint);

int net_ws_stream_endpoint_init(net_endpoint_t base_endpoint) {
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_underline = NULL;
    return 0;
}

void net_ws_stream_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        net_ws_endpoint_t underline = endpoint->m_underline;
        assert(underline->m_stream == endpoint);

        underline->m_stream = NULL;
        endpoint->m_underline = NULL;

        if (net_endpoint_is_active(underline->m_base_endpoint)) {
            if (net_endpoint_set_state(underline->m_base_endpoint, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(underline->m_base_endpoint, net_endpoint_state_deleting);
            }
        }
    }
}

void net_ws_stream_endpoint_calc_size(net_endpoint_t base_endpoint, net_endpoint_size_info_t size_info) {
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline && endpoint->m_underline->m_base_endpoint) {
        net_endpoint_calc_size(endpoint->m_underline->m_base_endpoint, size_info);
    }
    else {
        size_info->m_read = 0;
        size_info->m_write = 0;
    }
}

int net_ws_stream_endpoint_connect(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ws_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        if (net_ws_stream_endpoint_create_underline(base_endpoint) != 0) return -1;
    }

    if (net_ws_endpoint_set_runing_mode(endpoint->m_underline, net_ws_endpoint_runing_mode_cli) != 0) {
        CPE_ERROR(
            driver->m_em, "ws: stream: %s: connect: set undnline runing mode cli failed!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }
    
    net_endpoint_t base_underline = endpoint->m_underline->m_base_endpoint;
    if (net_endpoint_driver_debug(base_endpoint) > net_endpoint_protocol_debug(base_underline)) {
        net_endpoint_set_protocol_debug(base_underline, net_endpoint_driver_debug(base_endpoint));
    }
    
    net_address_t target_addr = net_endpoint_remote_address(base_endpoint);
    if (target_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "ws: stream: %s: connect: target addr not set!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    if (net_endpoint_set_remote_address(endpoint->m_underline->m_base_endpoint, target_addr) != 0) {
        CPE_ERROR(
            driver->m_em, "ws: stream: %s: connect: set remote address to underline fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    int rv = net_endpoint_connect(base_underline);

    if (rv != 0) {
        if (net_endpoint_state(base_endpoint) == net_endpoint_state_disable) {
            if (!net_endpoint_have_error(base_endpoint)) {
                net_endpoint_set_error(
                    base_endpoint,
                    net_endpoint_error_source(base_underline),
                    net_endpoint_error_no(base_underline),
                    net_endpoint_error_msg(base_underline));
            }
            net_endpoint_set_state(base_endpoint, net_endpoint_state_error);
        }
    }
    
    return rv;
}

int net_ws_stream_endpoint_update(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ws_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_endpoint_t base_underline = endpoint->m_underline ? endpoint->m_underline->m_base_endpoint : NULL;
    
    if (base_underline) {
        net_ws_stream_endpoint_update_readable(base_endpoint);
    }

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_read_closed:
        if (base_underline && net_endpoint_is_active(base_underline)) {
            if (net_endpoint_set_state(base_underline, net_endpoint_state_read_closed) != 0) {
                net_endpoint_set_state(base_underline, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_write_closed:
        if (base_underline && net_endpoint_is_active(base_underline)) {
            if (net_endpoint_set_state(base_underline, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(base_underline, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_disable:
        if (base_underline && net_endpoint_is_active(base_underline)) {
            if (net_endpoint_set_state(base_underline, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(base_underline, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_established:
        if (base_underline == NULL) {
            CPE_ERROR(
                driver->m_em, "ws: stream: %s: set no delay: no underline!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
            return -1;
        }

        if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) { /*有数据等待写入 */
            uint32_t buf_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
            void *  buf = NULL;
            if (net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_write, buf_size, &buf) != 0) {
                CPE_ERROR(
                    driver->m_em, "ws: stream: %s: peak data to send fail!, size=%d!",
                    net_endpoint_dump(net_ws_stream_driver_tmp_buffer(driver), base_endpoint), buf_size);
                return -1;
            }

            if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                char name_buf[128];
                cpe_str_dup(name_buf, sizeof(name_buf), net_endpoint_dump(net_ws_stream_driver_tmp_buffer(driver), base_endpoint));
                CPE_INFO(
                    driver->m_em, "ws: stream: %s: ==> %d data\n%s",
                    name_buf, buf_size,
                    mem_buffer_dump_data(net_ws_stream_driver_tmp_buffer(driver), buf, buf_size, 0));
            }
            
            if (net_ws_endpoint_send_msg_bin(endpoint->m_underline, buf, buf_size) != 0) {
                CPE_ERROR(
                    driver->m_em, "ws: stream: %s: send bin message fail, size=%d!",
                    net_endpoint_dump(net_ws_stream_driver_tmp_buffer(driver), base_endpoint), buf_size);
                return -1;
            }

            if (net_endpoint_is_active(base_endpoint)) {
                net_endpoint_buf_consume(base_endpoint, net_ep_buf_write, buf_size);
            }
            
            if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
        }

        assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);
        return 0;
    case net_endpoint_state_error:
        if (base_underline && net_endpoint_is_active(base_underline)) {
            if (net_endpoint_set_state(base_underline, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_underline, net_endpoint_state_deleting);
            }
        }
        return 0;
    default:
        return 0;
    }
}

int net_ws_stream_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_ws_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "ws: stream: %s: set no delay: no underline!",
            net_endpoint_dump(net_ws_stream_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_set_no_delay(endpoint->m_underline->m_base_endpoint, no_delay);
}

int net_ws_stream_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_ws_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "ws: stream: %s: get mss: no underline",
            net_endpoint_dump(net_ws_stream_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_get_mss(endpoint->m_underline->m_base_endpoint, mss);
}

net_ws_stream_endpoint_t
net_ws_stream_endpoint_create(net_ws_stream_driver_t driver, net_protocol_t protocol) {
    net_driver_t base_driver = net_driver_from_data(driver);
    
    net_endpoint_t base_endpoint = net_endpoint_create(base_driver, protocol, NULL);
    if (base_endpoint == NULL) return NULL;

    return net_endpoint_data(base_endpoint);
}

net_ws_stream_endpoint_t
net_ws_stream_endpoint_cast(net_endpoint_t base_endpoint) {
    net_driver_t driver = net_endpoint_driver(base_endpoint);
    return net_driver_endpoint_init_fun(driver) == net_ws_stream_endpoint_init
        ? net_endpoint_data(base_endpoint)
        : NULL;
}

net_endpoint_t
net_ws_stream_endpoint_underline(net_endpoint_t base_endpoint) {
    net_ws_stream_endpoint_t endpoint = net_ws_stream_endpoint_cast(base_endpoint);
    return endpoint && endpoint->m_underline ? endpoint->m_underline->m_base_endpoint : NULL;
}

net_endpoint_t
net_ws_stream_endpoint_base_endpoint(net_ws_stream_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

const char * net_ws_stream_endpoint_path(net_ws_stream_endpoint_t endpoint) {
    return endpoint->m_underline ? net_ws_endpoint_path(endpoint->m_underline) : NULL;
}

int net_ws_stream_endpoint_set_path(net_ws_stream_endpoint_t endpoint, const char * path) {
    net_ws_stream_driver_t driver = net_driver_data(net_endpoint_driver(endpoint->m_base_endpoint));
    
    if (endpoint->m_underline == NULL) {
        if (net_ws_stream_endpoint_create_underline(endpoint->m_base_endpoint) != 0) return -1;
    }

    return net_ws_endpoint_set_path(endpoint->m_underline, path);
}

net_address_t net_ws_stream_endpoint_host(net_ws_stream_endpoint_t endpoint) {
    return endpoint->m_underline ? net_ws_endpoint_host(endpoint->m_underline) : NULL;
}

int net_ws_stream_endpoint_set_host(net_ws_stream_endpoint_t endpoint, net_address_t host) {
    net_ws_stream_driver_t driver = net_driver_data(net_endpoint_driver(endpoint->m_base_endpoint));
    
    if (endpoint->m_underline == NULL) {
        if (net_ws_stream_endpoint_create_underline(endpoint->m_base_endpoint) != 0) return -1;
    }

    return net_ws_endpoint_set_host(endpoint->m_underline, host);
}

int net_ws_stream_endpoint_create_underline(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ws_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    assert(endpoint->m_underline == NULL);

    net_endpoint_t base_underline =
        net_endpoint_create(
            driver->m_underline_driver,
            driver->m_underline_protocol, NULL);
    if (base_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "ws: stream: %s: create undline ep fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    endpoint->m_underline = net_ws_endpoint_cast(base_underline);
    endpoint->m_underline->m_stream = endpoint;

    net_endpoint_set_expect_read(base_underline, net_endpoint_expect_read(base_endpoint));

    if (net_endpoint_driver_debug(base_endpoint) > net_endpoint_protocol_debug(base_underline)) {
        net_endpoint_set_protocol_debug(base_underline, net_endpoint_driver_debug(base_endpoint));
    }

    return 0;
}

void net_ws_stream_endpoint_update_readable(net_endpoint_t base_endpoint) {
    net_ws_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_endpoint_t base_underline = endpoint->m_underline ? endpoint->m_underline->m_base_endpoint : NULL;
    
    assert(base_underline != NULL);

    if (net_endpoint_expect_read(base_endpoint)) {
        if (!net_endpoint_expect_read(base_underline)) {
            if (net_endpoint_driver_debug(base_endpoint) >= 3) {
                CPE_INFO(
                    driver->m_em, "ws: stream: %s: read begin!",
                    net_endpoint_dump(net_ws_stream_driver_tmp_buffer(driver), base_endpoint));
            }

            net_endpoint_set_expect_read(base_underline, 1);
        }
    } else {
        if (net_endpoint_expect_read(base_underline)) {
            if (net_endpoint_driver_debug(base_endpoint) >= 3) {
                CPE_INFO(
                    driver->m_em, "ws: stream: %s: read stop!",
                    net_endpoint_dump(net_ws_stream_driver_tmp_buffer(driver), base_endpoint));
            }

            net_endpoint_set_expect_read(base_underline, 0);
        }
    }
}
