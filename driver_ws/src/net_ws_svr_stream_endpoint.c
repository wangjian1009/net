#include <assert.h>
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ws_svr_stream_endpoint_i.h"
#include "net_ws_svr_endpoint_i.h"

int net_ws_svr_stream_endpoint_init(net_endpoint_t base_endpoint) {
    net_ws_svr_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_underline = NULL;
    return 0;
}

void net_ws_svr_stream_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ws_svr_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        net_endpoint_free(endpoint->m_underline);
        assert(endpoint->m_underline == NULL);
    }
}

int net_ws_svr_stream_endpoint_update(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ws_svr_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_svr_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: set no delay: no underline!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_read_closed:
        return 0;
    case net_endpoint_state_write_closed:
        return 0;
    case net_endpoint_state_disable:
        if (net_endpoint_is_active(endpoint->m_underline)) {
            if (net_endpoint_set_state(endpoint->m_underline, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(endpoint->m_underline, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_established:
        if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) { /*有数据等待写入 */
            if (net_ws_svr_endpoint_write(endpoint->m_underline, base_endpoint, net_ep_buf_write) != 0) return -1;
            if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
        }

        assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);

        return 0;
    default:
        assert(0);
        return -1;
    }
}

int net_ws_svr_stream_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_ws_svr_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_svr_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: set no delay: no underline!",
            net_endpoint_dump(net_ws_svr_stream_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_set_no_delay(endpoint->m_underline, no_delay);
}

int net_ws_svr_stream_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_ws_svr_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ws_svr_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: get mss: no underline",
            net_endpoint_dump(net_ws_svr_stream_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_get_mss(endpoint->m_underline, mss);
}

net_endpoint_t
net_ws_svr_stream_endpoint_create(net_ws_svr_stream_driver_t driver, net_protocol_t protocol) {
    net_driver_t base_driver = net_driver_from_data(driver);
    
    net_endpoint_t base_endpoint = net_endpoint_create(base_driver, protocol, NULL);
    if (base_endpoint == NULL) return NULL;

    net_ws_svr_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_underline = net_endpoint_create(
        driver->m_underline_driver, driver->m_underline_protocol, NULL);
    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: create endpoint: create undline endpoint error!",
            net_driver_name(base_driver));
        net_endpoint_free(base_endpoint);
        return NULL;
    }

    net_ws_svr_endpoint_t underline = net_endpoint_protocol_data(endpoint->m_underline);
    underline->m_stream = endpoint;

    return base_endpoint;
}

net_endpoint_t
net_ws_svr_stream_endpoint_underline(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);

    if (net_driver_endpoint_init_fun(net_endpoint_driver(base_endpoint)) != net_ws_svr_stream_endpoint_init) {
        CPE_ERROR(
            net_schedule_em(schedule), "net: ws: %s: is not ws endpoint(dirver=%s), no underline",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_driver_name(net_endpoint_driver(base_endpoint)));
        return NULL;
    }

    net_ws_svr_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    return endpoint->m_underline;
}
