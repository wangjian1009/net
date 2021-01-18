#include <assert.h>
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_svr_endpoint_i.h"

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
    return 0;
}

int net_ssl_svr_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    return net_endpoint_set_no_delay(base_endpoint, no_delay);
}

int net_ssl_svr_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    return 0;
}
