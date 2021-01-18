#include <assert.h>
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_svr_endpoint_i.h"
#include "net_ssl_svr_underline_i.h"

static void net_ssl_svr_endpoint_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg);

int net_ssl_svr_endpoint_init(net_endpoint_t base_endpoint) {
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_underline = NULL;
    return 0;
}

void net_ssl_svr_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        net_endpoint_free(endpoint->m_underline);
        assert(endpoint->m_underline == NULL);
    }
}

void net_ssl_svr_endpoint_close(net_endpoint_t base_endpoint) {
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        if (net_endpoint_is_active(endpoint->m_underline)) {
            if (net_endpoint_set_state(endpoint->m_underline, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(endpoint->m_underline, net_endpoint_state_deleting);
            }
        }
    }
}

int net_ssl_svr_endpoint_update(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: set no delay: no underline!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }
    if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) { /*有数据等待写入 */
        if (net_ssl_svr_underline_write(endpoint->m_underline, base_endpoint, net_ep_buf_write) != 0) return -1;
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
    }

    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);

    return 0;
}

int net_ssl_svr_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_ssl_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: set no delay: no underline!",
            net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_set_no_delay(endpoint->m_underline, no_delay);
}

int net_ssl_svr_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_ssl_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: get mss: no underline",
            net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_get_mss(endpoint->m_underline, mss);
}
