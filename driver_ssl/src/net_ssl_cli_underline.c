#include <assert.h>
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_cli_underline_i.h"

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

    if (net_endpoint_state(base_underline) != net_endpoint_state_established) return -1;

	uint32_t data_size = net_endpoint_buf_size(base_underline, net_ep_buf_read);
    if (data_size == 0) return 0;
    
    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(base_underline, net_ep_buf_read, data_size, &data) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: peak data failed, size=%d!",
            net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline), data_size);
        return -1;
    }
    
    ERR_clear_error();
    int r = SSL_read(underline->m_ssl, data, data_size);
    if (r > 0) {
        if (net_endpoint_driver_debug(base_underline)) {
        }
    }
    else {
        int err = SSL_get_error(underline->m_ssl, r);
        switch (err) {
        case SSL_ERROR_WANT_READ:
            /* Can't read until underlying has more data. */
            /* if (bev_ssl->read_blocked_on_write) */
            /*     if (clear_rbow(bev_ssl) < 0) */
            /*         return OP_ERR | result; */
            break;
        case SSL_ERROR_WANT_WRITE:
            /* This read operation requires a write, and the
				 * underlying is full */
            /* if (!bev_ssl->read_blocked_on_write) */
            /*     if (set_rbow(bev_ssl) < 0) */
            /*         return OP_ERR | result; */
            break;
        case SSL_ERROR_ZERO_RETURN:
            /* 	/\* Possibly a clean shutdown. *\/ */
            /* 	if (SSL_get_shutdown(bev_ssl->ssl) & SSL_RECEIVED_SHUTDOWN) */
            /* 		event = BEV_EVENT_EOF; */
            /* 	else */
            /* 		dirty_shutdown = 1; */
            if (net_endpoint_set_state(base_underline, net_endpoint_state_disable) != 0) {
            }
            break;
        case SSL_ERROR_SYSCALL:
            /* 	/\* IO error; possibly a dirty shutdown. *\/ */
            /* 	if ((ret == 0 || ret == -1) && ERR_peek_error() == 0) */
            /* 		dirty_shutdown = 1; */
            /* 	put_error(bev_ssl, errcode); */
            break;
        default:
            net_ssl_cli_underline_dump_error(base_underline, err);
            net_endpoint_set_error(base_underline, net_endpoint_error_source_protocol, -1, "handshake start fail");
            //conn_closed(bev_ssl, BEV_EVENT_READING, err, r);
            break;
        }
    }
    
	/* 	if (r & (OP_BLOCKED|OP_ERR)) */
	/* 		break; */

	/* 	if (bev_ssl->bev.read_suspended) */
	/* 		break; */

	/* 	/\* Read all pending data.  This won't hit the network */
	/* 	 * again, and will (most importantly) put us in a state */
	/* 	 * where we don't need to read anything else until the */
	/* 	 * socket is readable again.  It'll potentially make us */
	/* 	 * overrun our read high-watermark (somewhat */
	/* 	 * regrettable).  The damage to the rate-limit has */
	/* 	 * already been done, since OpenSSL went and read a */
	/* 	 * whole SSL record anyway. *\/ */
	/* 	n_to_read = SSL_pending(bev_ssl->ssl); */

	/* 	/\* XXX This if statement is actually a bad bug, added to avoid */
	/* 	 * XXX a worse bug. */
	/* 	 * */
	/* 	 * The bad bug: It can potentially cause resource unfairness */
	/* 	 * by reading too much data from the underlying bufferevent; */
	/* 	 * it can potentially cause read looping if the underlying */
	/* 	 * bufferevent is a bufferevent_pair and deferred callbacks */
	/* 	 * aren't used. */
	/* 	 * */
	/* 	 * The worse bug: If we didn't do this, then we would */
	/* 	 * potentially not read any more from bev_ssl->underlying */
	/* 	 * until more data arrived there, which could lead to us */
	/* 	 * waiting forever. */
	/* 	 *\/ */
	/* 	if (!n_to_read && bev_ssl->underlying) */
	/* 		n_to_read = bytes_to_read(bev_ssl); */
	/* } */

	/* if (all_result_flags & OP_MADE_PROGRESS) { */
	/* 	struct bufferevent *bev = &bev_ssl->bev.bev; */

	/* 	bufferevent_trigger_nolock_(bev, EV_READ, 0); */
    
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
        if (net_endpoint_driver_debug(base_underline)) {
            CPE_INFO(
                driver->m_em, "net: ssl: %s: write: cache %d data in state %s!",
                net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline),
                net_endpoint_buf_size(from_ep, from_buf),
                net_endpoint_state_str(net_endpoint_state(base_underline)));
        }
        return net_endpoint_buf_append_from_other(
            base_underline, net_ep_buf_user1, from_ep, from_buf, 0);
    case net_endpoint_state_established:
        switch(underline->m_state) {
        case net_ssl_cli_underline_ssl_handshake:
            if (net_endpoint_driver_debug(base_underline)) {
                CPE_INFO(
                    driver->m_em, "net: ssl: %s: write: cache %d data in state %s.handshake!",
                    net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline),
                    net_endpoint_buf_size(from_ep, from_buf),
                    net_endpoint_state_str(net_endpoint_state(base_underline)));
            }
            return net_endpoint_buf_append_from_other(
                base_underline, net_ep_buf_user1, from_ep, from_buf, 0);
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
        if (underline->m_ssl_endpoint) {
            if (net_endpoint_set_state(
                    net_endpoint_from_data(underline->m_ssl_endpoint),
                    net_endpoint_state_established) != 0) return -1;
        }
    }
    else {
        int err = SSL_get_error(underline->m_ssl, r);
        switch (err) {
        case SSL_ERROR_WANT_WRITE:
            net_ssl_cli_underline_dump_error(base_underline, err);
            //stop_reading(bev_ssl);
            //return start_writing(bev_ssl);
            break;
        case SSL_ERROR_WANT_READ:
            //stop_writing(bev_ssl);
            //return start_reading(bev_ssl);
            return 0;
        default:
            net_ssl_cli_underline_dump_error(base_underline, err);
            break;
        }

        return -1;
    }

    return 0;
}

void net_ssl_cli_underline_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg)
{
    net_endpoint_t base_underline = arg;
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_underline));
    net_schedule_t schedule = net_endpoint_schedule(base_underline);
    
    if (net_endpoint_driver_debug(base_underline)) {
        char prefix[256];
        snprintf(
            prefix, sizeof(prefix), "net: ssl: %s: SSL: ",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_underline));
        net_ssl_dump_tls_info(
            driver->m_em, net_schedule_tmp_buffer(schedule), prefix,
            net_endpoint_driver_debug(base_underline) >= 2,
            write_p, version, content_type, buf, len, ssl);
    }
}

void net_ssl_cli_underline_dump_error(net_endpoint_t base_underline, int val) {
    net_schedule_t schedule = net_endpoint_schedule(base_underline);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_underline));

    CPE_ERROR(
        driver->m_em, "net: ssl: %s: SSL: Error was %s!",
        net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_underline),
        ERR_error_string(val, NULL));

    int err;
	while ((err = ERR_get_error())) {
		const char *msg = (const char*)ERR_reason_error_string(err);
		const char *lib = (const char*)ERR_lib_error_string(err);
		const char *func = (const char*)ERR_func_error_string(err);

        CPE_ERROR(
            driver->m_em, "net: ssl: %s: SSL:     %s in %s %s",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_underline),
            msg, lib, func);
	}
}

static int net_ssl_cli_underline_update_error(net_endpoint_t base_underline, int err, int r) {
    net_endpoint_set_error(base_underline, net_endpoint_error_source_protocol, -1, "handshake start fail");
    return 0;
}
