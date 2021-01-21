#include <assert.h>
#include "cpe/utils/error.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_cli_underline_i.h"

#define net_ssl_cli_underline_ep_write_cache net_ep_buf_user1
#define net_ssl_cli_underline_ep_read_cache net_ep_buf_user2

static void net_ssl_cli_underline_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg);

static int net_ssl_cli_underline_do_handshake(
    net_endpoint_t base_underline, net_ssl_cli_underline_t underline);

static void net_ssl_cli_underline_dump_error(net_endpoint_t base_underline, int val);

static int net_ssl_cli_underline_update_error(net_endpoint_t base_underline, int err, int r);

int net_ssl_cli_underline_init(net_endpoint_t base_underline) {
    net_ssl_cli_underline_t underline = net_endpoint_protocol_data(base_underline);
    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_cli_driver_t driver = protocol->m_driver;

    underline->m_ssl_endpoint = NULL;
    underline->m_state = net_ssl_cli_underline_ssl_handshake;

    underline->m_ssl = SSL_new(driver->m_ssl_ctx);
    if(underline->m_ssl == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: cli: endpoint init: create ssl fail");
        return -1;
    }
    SSL_set_msg_callback(underline->m_ssl, net_ssl_cli_underline_trace_cb);
    SSL_set_msg_callback_arg(underline->m_ssl, base_underline);
    SSL_set_connect_state(underline->m_ssl);

    BIO * bio = BIO_new(driver->m_bio_method);
    if (bio == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: cli: endpoint init: create bio fail");
        SSL_free(underline->m_ssl);
        return -1;
    }
	BIO_set_init(bio, 1);
	BIO_set_data(bio, base_underline);
	BIO_set_shutdown(bio, 0);
    SSL_set_bio(underline->m_ssl, bio, bio);
    
    return 0;
}

void net_ssl_cli_underline_fini(net_endpoint_t base_underline) {
    net_ssl_cli_underline_t underline = net_endpoint_protocol_data(base_underline);

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

int net_ssl_cli_underline_input(net_endpoint_t base_underline) {
    net_ssl_cli_underline_t underline = net_endpoint_protocol_data(base_underline);
    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_cli_driver_t driver = protocol->m_driver;

    if (underline->m_state == net_ssl_cli_underline_ssl_handshake) {
        if (net_ssl_cli_underline_do_handshake(base_underline, underline) != 0) return -1;
    }

READ_AGAIN:    
    if (net_endpoint_state(base_underline) != net_endpoint_state_established) return -1;

    uint32_t input_data_size = net_endpoint_buf_size(base_underline, net_ep_buf_read);
    if (input_data_size == 0) return 0;

    void * data = net_endpoint_buf_alloc(base_underline, &input_data_size);
    if (data == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: input: alloc data for read fail",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline));
        return -1;
    }

    ERR_clear_error();
    int r = SSL_read(underline->m_ssl, data, input_data_size);
    if (r > 0) {
        if (net_endpoint_buf_supply(base_underline, net_ssl_cli_underline_ep_read_cache, r) != 0) {
            CPE_ERROR(
                driver->m_em, "net: ssl: %s: input: load data to input cache fail",
                net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline));
            return -1;
        }

        net_endpoint_t base_endpoint =
            underline->m_ssl_endpoint ? net_endpoint_from_data(underline->m_ssl_endpoint) : NULL;

        if (base_endpoint
            && net_endpoint_state(base_endpoint) == net_endpoint_state_established)
        {
            if (net_endpoint_buf_append_from_other(
                    base_endpoint, net_ep_buf_read,
                    base_underline, net_ssl_cli_underline_ep_read_cache, 0) != 0)
            {
                CPE_ERROR(
                    driver->m_em, "net: ssl: %s: input: forward data to ssl ep fail",
                    net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline));
                return -1;
            }
            else {
                goto READ_AGAIN;
            }
        }
        else {
            net_endpoint_buf_clear(base_underline, net_ssl_cli_underline_ep_read_cache);
            CPE_ERROR(
                driver->m_em, "net: ssl: %s: input: no established ssl endpoint(state=%s), clear cached data",
                net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline),
                net_endpoint_state_str(net_endpoint_state(base_underline)));
        }
    }
    else {
        net_endpoint_buf_release(base_underline);

        int err = SSL_get_error(underline->m_ssl, r);
        return net_ssl_cli_underline_update_error(base_underline, err, r);
    }
    
    return 0;
}

int net_ssl_cli_underline_on_state_change(net_endpoint_t base_underline, net_endpoint_state_t from_state) {
    net_ssl_cli_underline_t underline = net_endpoint_protocol_data(base_underline);
    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_cli_driver_t driver = protocol->m_driver;

    net_endpoint_t base_endpoint =
        underline->m_ssl_endpoint ? net_endpoint_from_data(underline->m_ssl_endpoint) : NULL;
        
    switch(net_endpoint_state(base_underline)) {
    case net_endpoint_state_resolving:
        if (base_endpoint) {
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_resolving) != 0) return -1;
        }
        break;
    case net_endpoint_state_connecting:
        if (base_endpoint) {
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        }
        break;
    case net_endpoint_state_established:
        if (base_endpoint) {
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        }
        if (net_ssl_cli_underline_do_handshake(base_underline, underline) != 0) return -1;
        break;
    case net_endpoint_state_logic_error:
        if (base_endpoint) {
            net_endpoint_set_error(
                base_endpoint,
                net_endpoint_error_source(base_underline),
                net_endpoint_error_no(base_underline), net_endpoint_error_msg(base_underline));
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) return -1;
        }
        break;
    case net_endpoint_state_network_error:
        if (base_endpoint) {
            net_endpoint_set_error(
                base_endpoint,
                net_endpoint_error_source(base_underline),
                net_endpoint_error_no(base_underline), net_endpoint_error_msg(base_underline));
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
        }
        break;
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        if (base_endpoint) {
            net_endpoint_set_error(
                base_endpoint,
                net_endpoint_error_source_network,
                net_endpoint_network_errno_logic,
                "underline ep state error");
        }
        return -1;
    }
    
    return 0;
}

int net_ssl_cli_underline_write(
    net_endpoint_t base_underline, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf)
{
    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_cli_driver_t driver = protocol->m_driver;
    net_ssl_cli_underline_t underline = net_endpoint_protocol_data(base_underline);
    
    switch(net_endpoint_state(base_underline)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        if (net_endpoint_protocol_debug(base_underline)) {
            CPE_INFO(
                driver->m_em, "net: ssl: %s: write: cache %d data in state %s!",
                net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline),
                net_endpoint_buf_size(from_ep, from_buf),
                net_endpoint_state_str(net_endpoint_state(base_underline)));
        }

        if (base_underline == from_ep) {
            return net_endpoint_buf_append_from_self(
                base_underline, net_ssl_cli_underline_ep_write_cache, from_buf, 0);
        }
        else {
            return net_endpoint_buf_append_from_other(
                base_underline, net_ssl_cli_underline_ep_write_cache, from_ep, from_buf, 0);
        }
    case net_endpoint_state_established:
        switch(underline->m_state) {
        case net_ssl_cli_underline_ssl_handshake:
            if (net_endpoint_protocol_debug(base_underline)) {
                CPE_INFO(
                    driver->m_em, "net: ssl: %s: write: cache %d data in state %s.handshake!",
                    net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline),
                    net_endpoint_buf_size(from_ep, from_buf),
                    net_endpoint_state_str(net_endpoint_state(base_underline)));
            }

            if (base_underline == from_ep) {
                return net_endpoint_buf_append_from_self(
                    base_underline, net_ssl_cli_underline_ep_write_cache, from_buf, 0);
            }
            else {
                return net_endpoint_buf_append_from_other(
                    base_underline, net_ssl_cli_underline_ep_write_cache, from_ep, from_buf, 0);
            }
        case net_ssl_cli_underline_ssl_open:
            break;
        }
        break;
    case net_endpoint_state_logic_error:
    case net_endpoint_state_network_error:
    case net_endpoint_state_deleting:
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: write: can`t write in state %s!",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline),
            net_endpoint_state_str(net_endpoint_state(base_underline)));
        return -1;
    }

    uint32_t data_size = net_endpoint_buf_size(from_ep, from_buf);
    if (data_size == 0) return 0;

    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(from_ep, from_buf, data_size, &data) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: write: peak data fail!",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline));
        return -1;
    }

    int r = SSL_write(underline->m_ssl, data, data_size);
    if (r < 0) {
        int err = SSL_get_error(underline->m_ssl, r);
        return net_ssl_cli_underline_update_error(base_underline, err, r);
    }
    
    net_endpoint_buf_consume(from_ep, from_buf, r);

    if (net_endpoint_protocol_debug(base_underline)) {
        CPE_INFO(
            driver->m_em, "net: ssl: %s: ==> %d data!",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline), r);
    }
    
    return 0;
}

net_protocol_t
net_ssl_cli_underline_protocol_create(
    net_schedule_t schedule, const char * name, net_ssl_cli_driver_t driver)
{
    net_protocol_t base_protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_ssl_cli_underline_protocol), NULL, NULL,
            /*endpoint*/
            sizeof(struct net_ssl_cli_underline),
            net_ssl_cli_underline_init,
            net_ssl_cli_underline_fini,
            net_ssl_cli_underline_input,
            net_ssl_cli_underline_on_state_change,
            NULL);

    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_driver = driver;
    return base_protocol;
}

int net_ssl_cli_underline_do_handshake(net_endpoint_t base_underline, net_ssl_cli_underline_t underline) {
    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_cli_driver_t driver = protocol->m_driver;
    
    ERR_clear_error();
    int r = SSL_do_handshake(underline->m_ssl);
    if (r == 1) {
        underline->m_state = net_ssl_cli_underline_ssl_open;
        if (net_ssl_cli_underline_write(base_underline, base_underline, net_ssl_cli_underline_ep_write_cache) != 0) return -1;

        if (underline->m_ssl_endpoint) {
            if (net_endpoint_set_state(
                    net_endpoint_from_data(underline->m_ssl_endpoint),
                    net_endpoint_state_established) != 0) return -1;
        }
    }
    else {
        int err = SSL_get_error(underline->m_ssl, r);
        return net_ssl_cli_underline_update_error(base_underline, err, r);
    }

    return 0;
}

void net_ssl_cli_underline_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg)
{
    net_endpoint_t base_underline = arg;
    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_cli_driver_t driver = protocol->m_driver;

    if (net_endpoint_protocol_debug(base_underline)) {
        char prefix[256];
        snprintf(
            prefix, sizeof(prefix), "net: ssl: %s: SSL: ",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline));
        net_ssl_dump_tls_info(
            driver->m_em, net_ssl_cli_driver_tmp_buffer(driver), prefix,
            net_endpoint_protocol_debug(base_underline) >= 2,
            write_p, version, content_type, buf, len, ssl);
    }
}

void net_ssl_cli_underline_dump_error(net_endpoint_t base_underline, int val) {
    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_cli_driver_t driver = protocol->m_driver;

    CPE_ERROR(
        driver->m_em, "net: ssl: %s: SSL: Error was %s!",
        net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline),
        ERR_error_string(val, NULL));

    int err;
	while ((err = ERR_get_error())) {
		const char *msg = (const char*)ERR_reason_error_string(err);
		const char *lib = (const char*)ERR_lib_error_string(err);
		const char *func = (const char*)ERR_func_error_string(err);

        CPE_ERROR(
            driver->m_em, "net: ssl: %s: SSL:     %s in %s %s",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline),
            msg, lib, func);
	}
}

static int net_ssl_cli_underline_update_error(net_endpoint_t base_underline, int err, int r) {
    net_ssl_cli_underline_t underline = net_endpoint_protocol_data(base_underline);
    net_ssl_cli_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_cli_driver_t driver = protocol->m_driver;

    uint8_t dirty_shutdown = 0;
    
    switch (err) {
    case SSL_ERROR_WANT_READ:
        return 0;
    case SSL_ERROR_WANT_WRITE:
        return 0;
    case SSL_ERROR_ZERO_RETURN:
        /* Possibly a clean shutdown. */
        if (SSL_get_shutdown(underline->m_ssl) & SSL_RECEIVED_SHUTDOWN) {
            return net_endpoint_set_state(base_underline, net_endpoint_state_disable);
        }
        else {
            dirty_shutdown = 1;
        }
        break;
    case SSL_ERROR_SYSCALL:
        /* IO error; possibly a dirty shutdown. */
        if ((r == 0 || r == -1) && ERR_peek_error() == 0) {
            dirty_shutdown = 1;
        }
        /* 	put_error(bev_ssl, errcode); */
        break;
    default:
        net_ssl_cli_underline_dump_error(base_underline, err);
        net_endpoint_set_error(
            base_underline,
            net_endpoint_error_source_protocol,
            -1,
            "handshake start fail");
        return net_endpoint_set_state(base_underline, net_endpoint_state_logic_error);
    }
    return 0;
}
