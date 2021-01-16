#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_cli_endpoint_i.h"
#include "net_ssl_cli_underline_i.h"

int net_ssl_cli_endpoint_init(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_ssl = SSL_new(driver->m_ssl_ctx);
    if(endpoint->m_ssl == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: cli: endpoint init: create ssl fail");
        return -1;
    }

    BIO * bio = BIO_new(driver->m_bio_method);
    if (bio == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: cli: endpoint init: create bio fail");
        SSL_free(endpoint->m_ssl);
        return -1;
    }
	BIO_set_init(bio, 1);
	BIO_set_data(bio, base_endpoint);
	BIO_set_shutdown(bio, 0);
    
    endpoint->m_underline =
        net_endpoint_create(
            driver->m_underline_driver, driver->m_undline_protocol, NULL);
    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: connect: create inner ep fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }
    net_ssl_cli_undline_t underline = net_endpoint_protocol_data(endpoint->m_underline);
    underline->m_ssl_endpoint = endpoint;

    return 0;
}

void net_ssl_cli_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        net_endpoint_free(endpoint->m_underline);
        endpoint->m_underline = NULL;
    }

    if (endpoint->m_ssl) {
        SSL_free(endpoint->m_ssl);
        endpoint->m_ssl = NULL;
    }
}

int net_ssl_cli_endpoint_connect(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

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

    int rv = net_endpoint_connect(endpoint->m_underline);

    switch(net_endpoint_state(endpoint->m_underline)) {
    case net_endpoint_state_resolving:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_resolving) != 0) return -1;
        break;
    case net_endpoint_state_connecting:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        break;
    case net_endpoint_state_established:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        //TODO
        break;
    case net_endpoint_state_logic_error:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source(endpoint->m_underline),
            net_endpoint_error_no(endpoint->m_underline),
            net_endpoint_error_msg(endpoint->m_underline));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) return -1;
        break;
    case net_endpoint_state_network_error:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source(endpoint->m_underline),
            net_endpoint_error_no(endpoint->m_underline),
            net_endpoint_error_msg(endpoint->m_underline));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
        break;
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source_network,
            net_endpoint_network_errno_logic,
            "underline ep state error");
        return -1;
    }

    return rv;
}

void net_ssl_cli_endpoint_close(net_endpoint_t base_endpoint) {
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        //net_endpoint_close(endpoint->m_underline);
    }
}

int net_ssl_cli_endpoint_update(net_endpoint_t base_endpoint) {
    return 0;
}

int net_ssl_cli_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    return net_endpoint_set_no_delay(base_endpoint, no_delay);
}

int net_ssl_cli_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    return net_endpoint_get_mss(base_endpoint, mss);
}
