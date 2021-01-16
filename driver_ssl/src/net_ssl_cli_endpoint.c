#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_cli_endpoint_i.h"
#include "net_ssl_cli_underline_i.h"

static void net_ssl_cli_endpoint_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg);

int net_ssl_cli_endpoint_init(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_ssl = SSL_new(driver->m_ssl_ctx);
    if(endpoint->m_ssl == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: cli: endpoint init: create ssl fail");
        return -1;
    }
    SSL_set_msg_callback(endpoint->m_ssl, net_ssl_cli_endpoint_trace_cb);
    SSL_set_msg_callback_arg(endpoint->m_ssl, base_endpoint);

    BIO * bio = BIO_new(driver->m_bio_method);
    if (bio == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: cli: endpoint init: create bio fail");
        SSL_free(endpoint->m_ssl);
        return -1;
    }
	BIO_set_init(bio, 1);
	BIO_set_data(bio, base_endpoint);
	BIO_set_shutdown(bio, 0);
    SSL_set_bio(endpoint->m_ssl, bio, bio);
    
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

    endpoint->m_state = net_ssl_cli_endpoint_ssl_init;
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

        if (endpoint->m_state == net_ssl_cli_endpoint_ssl_init) {
            endpoint->m_state = net_ssl_cli_endpoint_ssl_handshake;
            if (net_ssl_cli_endpoint_handshake_start(base_endpoint, endpoint) != 0) return -1;
        }
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
        if (net_endpoint_is_active(endpoint->m_underline)) {
            if (net_endpoint_set_state(endpoint->m_underline, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(endpoint->m_underline, net_endpoint_state_deleting);
            }
        }
    }
}

int net_ssl_cli_endpoint_update(net_endpoint_t base_endpoint) {
    return 0;
}

int net_ssl_cli_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: set no delay: no underline!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    return net_endpoint_set_no_delay(endpoint->m_underline, no_delay);
}

int net_ssl_cli_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: get mss: no undline",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    return net_endpoint_get_mss(endpoint->m_underline, mss);
}

int net_ssl_cli_endpoint_handshake_start(net_endpoint_t base_endpoint, net_ssl_cli_endpoint_t endpoint) {
    ERR_clear_error();
    SSL_set_connect_state(endpoint->m_ssl);
    int r = SSL_do_handshake(endpoint->m_ssl);

    if (r != 1) {
        int err = SSL_get_error(endpoint->m_ssl, r);
        switch (err) {
        case SSL_ERROR_WANT_WRITE:
            //stop_reading(bev_ssl);
            //return start_writing(bev_ssl);
            break;
        case SSL_ERROR_WANT_READ:
            //stop_writing(bev_ssl);
            //return start_reading(bev_ssl);
            return 0;
        default:
            net_ssl_cli_endpoint_dump_error(base_endpoint, err);
            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_protocol, -1, "handshake start fail");
            break;
        }

        return -1;
    }

    return 0;
}

void net_ssl_cli_endpoint_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg)
{
    net_endpoint_t base_endpoint = arg;
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    
    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        char prefix[256];
        snprintf(
            prefix, sizeof(prefix), "net: ssl: %s: SSL: ",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        net_ssl_dump_tls_info(schedule, prefix, write_p, version, content_type, buf, len, ssl);
    }
}

void net_ssl_cli_endpoint_dump_error(net_endpoint_t base_endpoint, int val) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    CPE_ERROR(
        driver->m_em, "net: ssl: %s: SSL: Error was %s!",
        net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
        net_ssl_errno_str(val));

    int err;
	while ((err = ERR_get_error())) {
		const char *msg = (const char*)ERR_reason_error_string(err);
		const char *lib = (const char*)ERR_lib_error_string(err);
		const char *func = (const char*)ERR_func_error_string(err);

        CPE_ERROR(
            driver->m_em, "net: ssl: %s: SSL:     %s in %s %s",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            msg, lib, func);
	}
}
