#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_cli_endpoint_i.h"

int net_ssl_cli_endpoint_init(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_underline = net_endpoint_create(net_schedule_noop_protocol(schedule), driver->m_underline_driver);
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
    return 0;
}

void net_ssl_cli_endpoint_close(net_endpoint_t base_endpoint) {
}

int net_ssl_cli_endpoint_update(net_endpoint_t base_endpoint) {
    return 0;
}

int net_ssl_cli_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    return 0;
}

int net_ssl_cli_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    return 0;
}

