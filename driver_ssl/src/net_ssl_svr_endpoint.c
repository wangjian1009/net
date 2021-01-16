#include <assert.h>
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_svr_endpoint_i.h"

static void net_ssl_svr_endpoint_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg);

int net_ssl_svr_endpoint_init(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_ssl = SSL_new(driver->m_ssl_ctx);
    if(endpoint->m_ssl == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: svr: endpoint init: create ssl fail");
        return -1;
    }
    SSL_set_msg_callback(endpoint->m_ssl, net_ssl_svr_endpoint_trace_cb);
    SSL_set_msg_callback_arg(endpoint->m_ssl, base_endpoint);

    BIO * bio = BIO_new(driver->m_bio_method);
    if (bio == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: svr: endpoint init: create bio fail");
        SSL_free(endpoint->m_ssl);
        return -1;
    }
	BIO_set_init(bio, 1);
	BIO_set_data(bio, endpoint);
	BIO_set_shutdown(bio, 0);
    
    endpoint->m_underline = NULL;

    return 0;
}

void net_ssl_svr_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        net_endpoint_free(endpoint->m_underline);
        assert(endpoint->m_underline == NULL);
    }

    if (endpoint->m_ssl) {
        SSL_free(endpoint->m_ssl);
        endpoint->m_ssl = NULL;
    }
}

int net_ssl_svr_endpoint_connect(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    CPE_ERROR(
        driver->m_em, "net: ssl: %s: connect: not support connect",
        net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
    return -1;
}

void net_ssl_svr_endpoint_close(net_endpoint_t base_endpoint) {
}

int net_ssl_svr_endpoint_update(net_endpoint_t base_endpoint) {
    return 0;
}

int net_ssl_svr_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    return 0;
}

int net_ssl_svr_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    return 0;
}

void net_ssl_svr_endpoint_trace_cb(
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

void net_ssl_svr_endpoint_dump_error(net_endpoint_t base_endpoint, int val) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

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

