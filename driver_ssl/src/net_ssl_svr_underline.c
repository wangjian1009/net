#include <assert.h>
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ssl_svr_underline_i.h"

static void net_ssl_svr_underline_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg);

static void net_ssl_svr_underline_dump_error(net_endpoint_t base_underline, int val);

static int net_ssl_svr_underline_do_handshake(net_endpoint_t base_underline, net_ssl_svr_underline_t underline);

int net_ssl_svr_underline_init(net_endpoint_t base_underline) {
    net_ssl_svr_underline_t underline = net_endpoint_protocol_data(base_underline);
    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_svr_driver_t driver = protocol->m_driver;
    net_driver_t base_driver = net_driver_from_data(driver);

    underline->m_ssl_endpoint = NULL;
    underline->m_state = net_ssl_svr_underline_ssl_handshake;

    underline->m_ssl = SSL_new(driver->m_ssl_ctx);
    if(underline->m_ssl == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: endpoint init: create ssl fail",
            net_driver_name(base_driver));
        return -1;
    }
    SSL_set_msg_callback(underline->m_ssl, net_ssl_svr_underline_trace_cb);
    SSL_set_msg_callback_arg(underline->m_ssl, base_underline);
    SSL_set_accept_state(underline->m_ssl);

    BIO * bio = BIO_new(driver->m_bio_method);
    if (bio == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: endpoint init: create bio fail",
            net_driver_name(base_driver));
        SSL_free(underline->m_ssl);
        return -1;
    }
	BIO_set_init(bio, 1);
	BIO_set_data(bio, base_underline);
	BIO_set_shutdown(bio, 0);
    SSL_set_bio(underline->m_ssl, bio, bio);

    return 0;
}

void net_ssl_svr_underline_fini(net_endpoint_t base_underline) {
    net_ssl_svr_underline_t underline = net_endpoint_protocol_data(base_underline);

    if (underline->m_ssl_endpoint) {
        assert(underline->m_ssl_endpoint->m_underline == base_underline);
        underline->m_ssl_endpoint->m_underline = NULL;
        underline->m_ssl_endpoint = NULL;
    }

    if (underline->m_ssl) {
        SSL_free(underline->m_ssl);
        underline->m_ssl = NULL;
    }
}

int net_ssl_svr_underline_input(net_endpoint_t base_underline) {
    net_ssl_svr_underline_t underline = net_endpoint_protocol_data(base_underline);
    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_svr_driver_t driver = protocol->m_driver;

    switch(underline->m_state) {
    case net_ssl_svr_underline_ssl_handshake:
        if (net_ssl_svr_underline_do_handshake(base_underline, underline) != 0) return -1;
        break;
    case net_ssl_svr_underline_ssl_open:
        break;
    };
    
    return 0;
}

int net_ssl_svr_underline_on_state_change(net_endpoint_t base_underline, net_endpoint_state_t from_state) {
    switch(net_endpoint_state(base_underline)) {
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        assert(0);
        break;
    case net_endpoint_state_established:
        return 0;
    default:
        break;
    }
        
    net_ssl_svr_underline_t underline = net_endpoint_protocol_data(base_underline);
    if (underline->m_ssl_endpoint == NULL) return -1;
    
    assert(underline->m_ssl_endpoint->m_underline == base_underline);

    net_endpoint_t base_endpoint = net_endpoint_from_data(underline->m_ssl_endpoint);
    net_ssl_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    switch(net_endpoint_state(base_underline)) {
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
    case net_endpoint_state_established:
        assert(0);
        return 0;
    case net_endpoint_state_logic_error:
        net_endpoint_set_error(
            base_endpoint, net_endpoint_error_source(base_underline),
            net_endpoint_error_no(base_underline), net_endpoint_error_msg(base_underline));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) return -1;
        break;
    case net_endpoint_state_network_error:
        net_endpoint_set_error(
            base_endpoint, net_endpoint_error_source(base_underline),
            net_endpoint_error_no(base_underline), net_endpoint_error_msg(base_underline));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
        break;
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source_network, net_endpoint_network_errno_logic, "underline ep state error");
        return -1;
    }
    
    return 0;
}

net_protocol_t
net_ssl_svr_underline_protocol_create(net_schedule_t schedule, const char * name, net_ssl_svr_driver_t driver) {
    net_protocol_t base_protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_ssl_svr_underline_protocol), NULL, NULL,
            /*endpoint*/
            sizeof(struct net_ssl_svr_underline),
            net_ssl_svr_underline_init,
            net_ssl_svr_underline_fini,
            net_ssl_svr_underline_input,
            net_ssl_svr_underline_on_state_change,
            NULL);

    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_driver = driver;
    return base_protocol;
}

int net_ssl_svr_underline_do_handshake(net_endpoint_t base_underline, net_ssl_svr_underline_t underline) {
    ERR_clear_error();
    int r = SSL_do_handshake(underline->m_ssl);
    if (r != 1) {
        int err = SSL_get_error(underline->m_ssl, r);
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
            net_ssl_svr_underline_dump_error(base_underline, err);
            net_endpoint_set_error(base_underline, net_endpoint_error_source_protocol, -1, "handshake start fail");
            break;
        }

        return -1;
    }

    return 0;
}

void net_ssl_svr_underline_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg)
{
    net_endpoint_t base_underline = arg;
    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_svr_driver_t driver = protocol->m_driver;

    if (net_endpoint_protocol_debug(base_underline)) {
        char prefix[256];
        snprintf(
            prefix, sizeof(prefix), "net: ssl: %s: SSL: ",
            net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_underline));
        net_ssl_dump_tls_info(
            driver->m_em, net_ssl_svr_driver_tmp_buffer(driver), prefix,
            net_endpoint_protocol_debug(base_underline) >= 2,
            write_p, version, content_type, buf, len, ssl);
    }
}

void net_ssl_svr_underline_dump_error(net_endpoint_t base_underline, int val) {
    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_svr_driver_t driver = protocol->m_driver;

    CPE_ERROR(
        driver->m_em, "net: ssl: %s: SSL: Error was %s!",
        net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_underline),
        ERR_error_string(val, NULL));

    int err;
	while ((err = ERR_get_error())) {
		const char *msg = (const char*)ERR_reason_error_string(err);
		const char *lib = (const char*)ERR_lib_error_string(err);
		const char *func = (const char*)ERR_func_error_string(err);

        CPE_ERROR(
            driver->m_em, "net: ssl: %s: SSL:     %s in %s %s",
            net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_underline),
            msg, lib, func);
	}
}

