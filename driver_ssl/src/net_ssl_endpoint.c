#include <assert.h>
#include "cpe/utils/error.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ssl_endpoint_i.h"
#include "net_ssl_stream_endpoint_i.h"

#define net_ssl_endpoint_ep_write_cache net_ep_buf_user1
#define net_ssl_endpoint_ep_read_cache net_ep_buf_user2

static void net_ssl_endpoint_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg);

static int net_ssl_endpoint_do_handshake(
    net_endpoint_t base_endpoint, net_ssl_endpoint_t endpoint);

static void net_ssl_endpoint_dump_error(net_endpoint_t base_endpoint, int val);

static int net_ssl_endpoint_update_error(net_endpoint_t base_endpoint, int err, int r);

int net_ssl_endpoint_init(net_endpoint_t base_endpoint) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_stream = NULL;
    endpoint->m_state = net_ssl_endpoint_state_init;
    endpoint->m_ssl = NULL;
    
    return 0;
}

void net_ssl_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);

    if (endpoint->m_stream) {
        net_ssl_stream_endpoint_t stream = endpoint->m_stream;
        assert(stream->m_underline == endpoint);

        endpoint->m_stream = NULL;
        stream->m_underline = NULL;

        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
        }
    }

    if (endpoint->m_ssl) {
        SSL_free(endpoint->m_ssl);
        endpoint->m_ssl = NULL;
    }
}

int net_ssl_endpoint_input(net_endpoint_t base_endpoint) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_state == net_ssl_endpoint_state_handshake) {
        if (net_ssl_endpoint_do_handshake(base_endpoint, endpoint) != 0) return -1;
    }

READ_AGAIN:    
    if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return -1;

    uint32_t input_data_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_read);
    if (input_data_size == 0) return 0;

    void * data = net_endpoint_buf_alloc_at_least(base_endpoint, &input_data_size);
    if (data == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ssl: %s: input: alloc data for read fail",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    ERR_clear_error();
    int r = SSL_read(endpoint->m_ssl, data, input_data_size);
    if (r > 0) {
        if (net_endpoint_buf_supply(base_endpoint, net_ssl_endpoint_ep_read_cache, r) != 0) {
            CPE_ERROR(
                protocol->m_em, "net: ssl: %s: input: load data to input cache fail",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
            return -1;
        }

        net_endpoint_t base_endpoint =
            endpoint->m_stream ? net_endpoint_from_data(endpoint->m_stream) : NULL;

        if (base_endpoint
            && net_endpoint_state(base_endpoint) == net_endpoint_state_established)
        {
            if (net_endpoint_buf_append_from_other(
                    base_endpoint, net_ep_buf_read,
                    base_endpoint, net_ssl_endpoint_ep_read_cache, 0) != 0)
            {
                CPE_ERROR(
                    protocol->m_em, "net: ssl: %s: input: forward data to ssl ep fail",
                    net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
                return -1;
            }
            else {
                goto READ_AGAIN;
            }
        }
        else {
            net_endpoint_buf_clear(base_endpoint, net_ssl_endpoint_ep_read_cache);
            CPE_ERROR(
                protocol->m_em, "net: ssl: %s: input: no established ssl endpoint(state=%s), clear cached data",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
                net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        }
    }
    else {
        net_endpoint_buf_release(base_endpoint);

        int err = SSL_get_error(endpoint->m_ssl, r);
        return net_ssl_endpoint_update_error(base_endpoint, err, r);
    }
    
    return 0;
}

int net_ssl_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    net_endpoint_t base_stream =
        endpoint->m_stream ? net_endpoint_from_data(endpoint->m_stream) : NULL;
        
    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_resolving:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_resolving) != 0) return -1;
        }
        break;
    case net_endpoint_state_connecting:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) return -1;
        }
        break;
    case net_endpoint_state_established:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) return -1;
        }
        /* if (endpoint->m_runing_mode == net_ws_endpoint_runing_mode_cli) { */
        /*     if (net_ws_endpoint_send_handshake(base_endpoint, endpoint) != 0) return -1; */
        /* } */
        break;
    case net_endpoint_state_error:
        if (base_stream) {
            net_endpoint_set_error(
                base_stream,
                net_endpoint_error_source(base_endpoint),
                net_endpoint_error_no(base_endpoint), net_endpoint_error_msg(base_endpoint));
            if (net_endpoint_set_state(base_stream, net_endpoint_state_error) != 0) return -1;
        }
        break;
    case net_endpoint_state_read_closed:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_read_closed) != 0) return -1;
        }
        break;
    case net_endpoint_state_write_closed:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_write_closed) != 0) return -1;
        }
        break;
    case net_endpoint_state_disable:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_disable) != 0) return -1;
        }
        break;
    case net_endpoint_state_deleting:
        break;
    }
    
    return 0;
}

int net_ssl_endpoint_write(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf)
{
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        if (net_endpoint_protocol_debug(base_endpoint)) {
            CPE_INFO(
                protocol->m_em, "net: ssl: %s: write: cache %d data in state %s!",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
                net_endpoint_buf_size(from_ep, from_buf),
                net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        }

        if (base_endpoint == from_ep) {
            return net_endpoint_buf_append_from_self(
                base_endpoint, net_ssl_endpoint_ep_write_cache, from_buf, 0);
        }
        else {
            return net_endpoint_buf_append_from_other(
                base_endpoint, net_ssl_endpoint_ep_write_cache, from_ep, from_buf, 0);
        }
    case net_endpoint_state_established:
        switch(endpoint->m_state) {
        case net_ssl_endpoint_state_init:
        case net_ssl_endpoint_state_handshake:
            if (net_endpoint_protocol_debug(base_endpoint)) {
                CPE_INFO(
                    protocol->m_em, "net: ssl: %s: write: cache %d data in state %s.handshake!",
                    net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
                    net_endpoint_buf_size(from_ep, from_buf),
                    net_endpoint_state_str(net_endpoint_state(base_endpoint)));
            }

            if (base_endpoint == from_ep) {
                return net_endpoint_buf_append_from_self(
                    base_endpoint, net_ssl_endpoint_ep_write_cache, from_buf, 0);
            }
            else {
                return net_endpoint_buf_append_from_other(
                    base_endpoint, net_ssl_endpoint_ep_write_cache, from_ep, from_buf, 0);
            }
        case net_ssl_endpoint_state_streaming:
            break;
        }
        break;
    case net_endpoint_state_error:
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
    case net_endpoint_state_deleting:
        CPE_ERROR(
            protocol->m_em, "net: ssl: %s: write: can`t write in state %s!",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
            net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        return -1;
    }

    uint32_t data_size = net_endpoint_buf_size(from_ep, from_buf);
    if (data_size == 0) return 0;

    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(from_ep, from_buf, data_size, &data) != 0) {
        CPE_ERROR(
            protocol->m_em, "net: ssl: %s: write: peak data fail!",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    int r = SSL_write(endpoint->m_ssl, data, data_size);
    if (r < 0) {
        int err = SSL_get_error(endpoint->m_ssl, r);
        return net_ssl_endpoint_update_error(base_endpoint, err, r);
    }
    
    net_endpoint_buf_consume(from_ep, from_buf, r);

    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "net: ssl: %s: ==> %d data!",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint), r);
    }
    
    return 0;
}

int net_ssl_endpoint_do_handshake(net_endpoint_t base_endpoint, net_ssl_endpoint_t endpoint) {
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    
    ERR_clear_error();
    int r = SSL_do_handshake(endpoint->m_ssl);
    if (r == 1) {
        net_ssl_endpoint_set_state(endpoint, net_ssl_endpoint_state_streaming);
        if (net_ssl_endpoint_write(base_endpoint, base_endpoint, net_ssl_endpoint_ep_write_cache) != 0) return -1;

        if (endpoint->m_stream) {
            if (net_endpoint_set_state(
                    net_endpoint_from_data(endpoint->m_stream),
                    net_endpoint_state_established) != 0) return -1;
        }
    }
    else {
        int err = SSL_get_error(endpoint->m_ssl, r);
        return net_ssl_endpoint_update_error(base_endpoint, err, r);
    }

    return 0;
}

void net_ssl_endpoint_trace_cb(
    int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg)
{
    net_ssl_endpoint_t endpoint = arg;
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        char prefix[256];
        snprintf(
            prefix, sizeof(prefix), "net: ssl: %s: SSL: ",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        net_ssl_dump_tls_info(
            protocol->m_em, net_ssl_protocol_tmp_buffer(protocol), prefix,
            net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2,
            write_p, version, content_type, buf, len, ssl);
    }
}

void net_ssl_endpoint_dump_error(net_endpoint_t base_endpoint, int val) {
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    CPE_ERROR(
        protocol->m_em, "net: ssl: %s: SSL: Error was %s!",
        net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
        ERR_error_string(val, NULL));

    int err;
	while ((err = ERR_get_error())) {
		const char *msg = (const char*)ERR_reason_error_string(err);
		const char *lib = (const char*)ERR_lib_error_string(err);
		const char *func = (const char*)ERR_func_error_string(err);

        CPE_ERROR(
            protocol->m_em, "net: ssl: %s: SSL:     %s in %s %s",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
            msg, lib, func);
	}
}

static int net_ssl_endpoint_update_error(net_endpoint_t base_endpoint, int err, int r) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    uint8_t dirty_shutdown = 0;
    
    switch (err) {
    case SSL_ERROR_WANT_READ:
        return 0;
    case SSL_ERROR_WANT_WRITE:
        return 0;
    case SSL_ERROR_ZERO_RETURN:
        /* Possibly a clean shutdown. */
        if (SSL_get_shutdown(endpoint->m_ssl) & SSL_RECEIVED_SHUTDOWN) {
            return net_endpoint_set_state(base_endpoint, net_endpoint_state_disable);
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
        net_ssl_endpoint_dump_error(base_endpoint, err);
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source_ssl,
            -1,
            "handshake start fail");
        return net_endpoint_set_state(base_endpoint, net_endpoint_state_error);
    }
    return 0;
}

int net_ssl_endpoint_set_runing_mode(net_ssl_endpoint_t endpoint, net_ssl_endpoint_runing_mode_t runing_mode) {
    if (endpoint->m_runing_mode == runing_mode) return 0;

    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "net: ssl: %s: runing-mode: %s ==> %s",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ssl_endpoint_runing_mode_str(endpoint->m_runing_mode),
            net_ssl_endpoint_runing_mode_str(runing_mode));
    }

    if (endpoint->m_state != net_ssl_endpoint_state_init) {
        if (net_ssl_endpoint_set_state(endpoint, net_ssl_endpoint_state_init) != 0) return -1;
    }

    endpoint->m_runing_mode = runing_mode;

    if (endpoint->m_ssl) {
        SSL_free(endpoint->m_ssl);
        endpoint->m_ssl = NULL;
    }

    switch(endpoint->m_runing_mode) {
    case net_ssl_endpoint_runing_mode_init:
        break;
    case net_ssl_endpoint_runing_mode_cli:
        break;
    case net_ssl_endpoint_runing_mode_svr:
        break;
    }
    
    endpoint->m_ssl = SSL_new(protocol->m_ssl_ctx);
    if(endpoint->m_ssl == NULL) {
        CPE_ERROR(protocol->m_em, "net: ssl: cli: endpoint init: create ssl fail");
        return -1;
    }
    SSL_set_msg_callback(endpoint->m_ssl, net_ssl_endpoint_trace_cb);
    SSL_set_msg_callback_arg(endpoint->m_ssl, endpoint);
    SSL_set_connect_state(endpoint->m_ssl);

    BIO * bio = BIO_new(protocol->m_bio_method);
    if (bio == NULL) {
        CPE_ERROR(protocol->m_em, "net: ssl: cli: endpoint init: create bio fail");
        SSL_free(endpoint->m_ssl);
        return -1;
    }
	BIO_set_init(bio, 1);
	BIO_set_data(bio, endpoint);
	BIO_set_shutdown(bio, 0);
    SSL_set_bio(endpoint->m_ssl, bio, bio);
    
    return 0;
}

int net_ssl_endpoint_set_state(net_ssl_endpoint_t endpoint, net_ssl_endpoint_state_t state) {
    if (endpoint->m_state == state) return 0;

    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    assert(endpoint->m_runing_mode != net_ssl_endpoint_runing_mode_init);
    
    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "net: ssl: %s: state: %s ==> %s",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ssl_endpoint_state_str(endpoint->m_state),
            net_ssl_endpoint_state_str(state));
    }

    endpoint->m_state = state;

    switch(state) {
    case net_ssl_endpoint_state_init:
        break;
    case net_ssl_endpoint_state_handshake:
        break;
    case net_ssl_endpoint_state_streaming:
        if (endpoint->m_stream) {
            net_endpoint_t base_stream= endpoint->m_stream->m_base_endpoint;
            if (net_endpoint_set_state(base_stream, net_endpoint_state_established) != 0) {
                net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
            }
        }

        break;
    }

    return 0;
}
