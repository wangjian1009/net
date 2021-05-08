#include <assert.h>
#include "mbedtls/error.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/error.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_ssl_endpoint_i.h"
#include "net_ssl_stream_endpoint_i.h"
#include "net_ssl_utils.h"

#define net_ssl_endpoint_ep_write_cache net_ep_buf_user1
#define net_ssl_endpoint_ep_read_cache net_ep_buf_user2

static int net_ssl_endpoint_bio_read(void * ctx, unsigned char *out, size_t outlen);
static int net_ssl_endpoint_bio_write(void * ctx, const unsigned char *in, size_t inlen);
static void net_ssl_endpoint_ssl_fini(net_ssl_endpoint_t endpoint);
static int net_ssl_endpoint_ssl_init(net_ssl_endpoint_t endpoint);
static int net_ssl_endpoint_ssl_verify(void *ctx, mbedtls_x509_crt *crt, int depth, uint32_t *flags);

static int net_ssl_endpoint_do_handshake(
    net_endpoint_t base_endpoint, net_ssl_endpoint_t endpoint);

static int net_ssl_endpoint_on_handshake_success(
    net_endpoint_t base_endpoint, net_ssl_endpoint_t endpoint);

static int net_ssl_endpoint_prepare_client_hello(
    net_endpoint_t base_endpoint, net_ssl_endpoint_t endpoint);

static void net_ssl_endpoint_dump_error(net_endpoint_t base_endpoint, int val);

static int net_ssl_endpoint_update_error(net_endpoint_t base_endpoint, int r);

void net_ssl_endpoint_free(net_ssl_endpoint_t endpoint) {
    net_endpoint_free(endpoint->m_base_endpoint);
}

net_ssl_endpoint_t
net_ssl_endpoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_ssl_endpoint_init
        ? net_endpoint_protocol_data(base_endpoint)
        : NULL;
}

net_ssl_stream_endpoint_t net_ssl_endpoint_stream(net_ssl_endpoint_t endpoint) {
    return endpoint->m_stream;
}

net_ssl_endpoint_runing_mode_t net_ssl_endpoint_runing_mode(net_ssl_endpoint_t endpoint) {
    return endpoint->m_runing_mode;
}

net_ssl_endpoint_state_t net_ssl_endpoint_state(net_ssl_endpoint_t endpoint) {
    return endpoint->m_state;
}

net_endpoint_t net_ssl_endpoint_base_endpoint(net_ssl_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

int net_ssl_endpoint_close(net_ssl_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = endpoint->m_base_endpoint;
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
        return net_endpoint_set_state(base_endpoint, net_endpoint_state_disable);
    case net_endpoint_state_established:
        break;
    case net_endpoint_state_disable:
    case net_endpoint_state_error:
    case net_endpoint_state_deleting:
        return 0;
    }
    
    int ret = mbedtls_ssl_close_notify(endpoint->m_ssl);
    if (ret < 0) {
        char error_buf[1024];
        CPE_ERROR(
            protocol->m_em, "ssl: %s: ssl close: mbedtls_ssl_close_notify() returned -0x%04X(%s)",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            -ret, net_ssl_strerror(error_buf, sizeof(error_buf), ret));
        return net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_disable);
    }

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "ssl: %s: close notify",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
    }

    return 0;
}

int net_ssl_endpoint_init(net_endpoint_t base_endpoint) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_stream = NULL;
    endpoint->m_runing_mode = net_ssl_endpoint_runing_mode_init;
    endpoint->m_state = net_ssl_endpoint_state_init;
    endpoint->m_ssl_config = NULL;
    endpoint->m_ssl = NULL;

    //net_endpoint_set_protocol_debug(base_endpoint, 2);
    return 0;
}

void net_ssl_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_stream) {
        net_ssl_stream_endpoint_t stream = endpoint->m_stream;
        assert(stream->m_underline == endpoint);

        endpoint->m_stream = NULL;
        stream->m_underline = NULL;

        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
        }
    }

    net_ssl_endpoint_ssl_fini(endpoint);
}

int net_ssl_endpoint_input(net_endpoint_t base_endpoint) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_runing_mode == net_ssl_endpoint_runing_mode_init) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: input in runing-mode %s, error",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
            net_ssl_endpoint_runing_mode_str(endpoint->m_runing_mode));
        return -1;
    }

    if (endpoint->m_state == net_ssl_endpoint_state_init) {
        if (net_ssl_endpoint_set_state(endpoint, net_ssl_endpoint_state_handshake) != 0) return -1;
    }
    
    if (endpoint->m_state == net_ssl_endpoint_state_handshake) {
        if (net_ssl_endpoint_do_handshake(base_endpoint, endpoint) != 0) return -1;

        if (endpoint->m_state == net_ssl_endpoint_state_handshake) return 0;
    }

READ_AGAIN:    
    if (!net_endpoint_is_readable(base_endpoint)) return -1;

    uint32_t input_data_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_read);
    input_data_size += endpoint->m_ssl->in_left;

    void * data = net_endpoint_buf_alloc_at_least(base_endpoint, &input_data_size);
    if (data == NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: input: alloc data for read fail",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    int r = mbedtls_ssl_read(endpoint->m_ssl, data, input_data_size);
    if (r > 0) {
        if (net_endpoint_buf_supply(base_endpoint, net_ssl_endpoint_ep_read_cache, r) != 0) {
            CPE_ERROR(
                protocol->m_em, "ssl: %s: input: load data to input cache fail",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
            return -1;
        }

        net_endpoint_t base_stream =
            endpoint->m_stream ? net_endpoint_from_data(endpoint->m_stream) : NULL;

        if (base_stream && net_endpoint_is_readable(base_stream)) {
            if (net_endpoint_driver_debug(base_stream) >= 2) {
                CPE_INFO(
                    protocol->m_em, "ssl: %s: <<< %d data",
                    net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_stream),
                    net_endpoint_buf_size(base_endpoint, net_ssl_endpoint_ep_read_cache));
            }
            
            if (net_endpoint_buf_append_from_other(
                    base_stream, net_ep_buf_read,
                    base_endpoint, net_ssl_endpoint_ep_read_cache, 0) != 0)
            {
                CPE_ERROR(
                    protocol->m_em, "ssl: %s: input: forward data to ssl ep fail",
                    net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_stream));
                return -1;
            }
            else {
                goto READ_AGAIN;
            }
        }
        else {
            net_endpoint_buf_clear(base_endpoint, net_ssl_endpoint_ep_read_cache);
            CPE_ERROR(
                protocol->m_em, "ssl: %s: input: no established ssl endpoint(state=%s), clear cached data",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
                net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        }
    }
    else {
        net_endpoint_buf_release(base_endpoint);
        return net_ssl_endpoint_update_error(base_endpoint, r);
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
        if (endpoint->m_runing_mode == net_ssl_endpoint_runing_mode_cli) {
            if (net_ssl_endpoint_prepare_client_hello(base_endpoint, endpoint) != 0) return -1;
            if (net_ssl_endpoint_do_handshake(base_endpoint, endpoint) != 0) return -1;
        }
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

void net_ssl_endpoint_calc_size(net_endpoint_t base_endpoint, net_endpoint_size_info_t size_info) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    
    size_info->m_read = net_endpoint_buf_size(base_endpoint, net_ep_buf_read);
    size_info->m_write = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);

    if (endpoint->m_ssl) {
        size_info->m_read += endpoint->m_ssl->in_left;
        size_info->m_write += endpoint->m_ssl->out_left;
    }
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
        /* if (net_endpoint_protocol_debug(base_endpoint)) { */
            CPE_INFO(
                protocol->m_em, "ssl: %s: write: cache %d data in state %s!",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
                net_endpoint_buf_size(from_ep, from_buf),
                net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        /* } */

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
                    protocol->m_em, "ssl: %s: write: cache %d data in state %s.handshake!",
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
            protocol->m_em, "ssl: %s: write: can`t write in state %s!",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
            net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        return -1;
    }

    uint32_t data_size = net_endpoint_buf_size(from_ep, from_buf);
    if (data_size == 0) return 0;

    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(from_ep, from_buf, data_size, &data) != 0) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: write: peak data fail!",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    int r = mbedtls_ssl_write(endpoint->m_ssl, data, data_size);
    if (r < 0) {
        return net_ssl_endpoint_update_error(base_endpoint, r);
    }

    if (net_endpoint_is_active(from_ep)) {
        net_endpoint_buf_consume(from_ep, from_buf, r);
    }
    
    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "ssl: %s: ==> %d data!",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint), r);
    }
    
    return 0;
}

int net_ssl_endpoint_do_handshake(net_endpoint_t base_endpoint, net_ssl_endpoint_t endpoint) {
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    int r = mbedtls_ssl_handshake(endpoint->m_ssl);
    if (r < 0) {
        return net_ssl_endpoint_update_error(base_endpoint, r);
    }
    
    if (r == 0) {
        if (net_ssl_endpoint_on_handshake_success(base_endpoint, endpoint) != 0) return -1;
        
        net_ssl_endpoint_set_state(endpoint, net_ssl_endpoint_state_streaming);
        if (net_endpoint_is_writeable(base_endpoint)
            && !net_endpoint_buf_is_empty(base_endpoint, net_ssl_endpoint_ep_write_cache))
        {
            if (net_ssl_endpoint_write(base_endpoint, base_endpoint, net_ssl_endpoint_ep_write_cache) != 0) return -1;
            if (!net_endpoint_is_active(base_endpoint)) return -1;
        }
        
        if (endpoint->m_stream) {
            if (net_endpoint_set_state(
                    net_endpoint_from_data(endpoint->m_stream),
                    net_endpoint_state_established) != 0) return -1;
        }
    }

    return 0;
}

int net_ssl_endpoint_on_handshake_success(net_endpoint_t base_endpoint, net_ssl_endpoint_t endpoint) {
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    
    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "ssl: %s: handshake complete, cipher is %s",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
            mbedtls_ssl_get_ciphersuite(endpoint->m_ssl));
    }

    const mbedtls_x509_crt * peercert = mbedtls_ssl_get_peer_cert(endpoint->m_ssl);
    if (peercert) {
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
            char ep_name[256];
            cpe_str_dup(ep_name, sizeof(ep_name), net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));

            CPE_INFO(
                protocol->m_em, "ssl: %s: handshake complete, Dumping cert info:\n%s",
                ep_name, net_ssl_dump_cert_info(net_ssl_protocol_tmp_buffer(protocol), peercert));
        }
    }

    return 0;
}

static int net_ssl_endpoint_prepare_client_hello(
    net_endpoint_t base_endpoint, net_ssl_endpoint_t endpoint)
{
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    int rv;

    net_address_t address = net_endpoint_remote_address(base_endpoint);
    if (address && net_address_type(address) == net_address_domain) {
        if ((rv = mbedtls_ssl_set_hostname(endpoint->m_ssl, net_address_domain_url(address))) != 0) {
            char error_buf[1024];
            CPE_ERROR(
                protocol->m_em, "ssl: %s: ssl client hello: mbedtls_ssl_set_hostname, -0x%04X(%s)",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                -rv, net_ssl_strerror(error_buf, sizeof(error_buf), rv));
            return -1;
        }
    }
    
    return 0;
}

/* void net_ssl_endpoint_trace_cb( */
/*     int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg) */
/* { */
/*     net_ssl_endpoint_t endpoint = arg; */
/*     net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint)); */

/*     if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) { */
/*         char prefix[256]; */
/*         snprintf( */
/*             prefix, sizeof(prefix), "ssl: %s: SSL: ", */
/*             net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint)); */
/*         net_ssl_dump_tls_info( */
/*             protocol->m_em, net_ssl_protocol_tmp_buffer(protocol), prefix, */
/*             net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2, */
/*             write_p, version, content_type, buf, len, ssl); */
/*     } */
/* } */

void net_ssl_endpoint_dump_error(net_endpoint_t base_endpoint, int val) {
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    char error_buf[1024];
    CPE_ERROR(
        protocol->m_em, "ssl: %s: SSL: Error was %s!",
        net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
        net_ssl_strerror(error_buf, sizeof(error_buf), val));
}

static int net_ssl_endpoint_update_error(net_endpoint_t base_endpoint, int ssl_error) {
    net_ssl_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    switch (ssl_error) {
    case MBEDTLS_ERR_SSL_WANT_READ:
        if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ssl: %s: want read, input-size=%d",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint),
                net_endpoint_buf_size(base_endpoint, net_ep_buf_read));
        }
        return 0;
    case MBEDTLS_ERR_SSL_WANT_WRITE:
        if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ssl: %s: want write",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
        }
        return 0;
    case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
        if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ssl: %s: peer close notify received",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), base_endpoint));
        }
        return net_endpoint_set_state(base_endpoint, net_endpoint_state_disable);
    default:
        net_ssl_endpoint_dump_error(base_endpoint, ssl_error);
        char error_buf[1024];
        net_endpoint_set_error(
            base_endpoint, net_endpoint_error_source_ssl, ssl_error,
            net_ssl_strerror(error_buf, sizeof(error_buf), ssl_error));
        return net_endpoint_set_state(base_endpoint, net_endpoint_state_error);
    }
    return 0;
}

int net_ssl_endpoint_set_runing_mode(net_ssl_endpoint_t endpoint, net_ssl_endpoint_runing_mode_t runing_mode) {
    if (endpoint->m_runing_mode == runing_mode) return 0;

    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "ssl: %s: runing-mode: %s ==> %s",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ssl_endpoint_runing_mode_str(endpoint->m_runing_mode),
            net_ssl_endpoint_runing_mode_str(runing_mode));
    }

    if (endpoint->m_state != net_ssl_endpoint_state_init) {
        if (net_ssl_endpoint_set_state(endpoint, net_ssl_endpoint_state_init) != 0) return -1;
    }

    endpoint->m_runing_mode = runing_mode;

    if (endpoint->m_runing_mode != net_ssl_endpoint_runing_mode_init) {
        if (net_ssl_endpoint_ssl_init(endpoint) != 0) {
            endpoint->m_runing_mode = net_ssl_endpoint_runing_mode_init;
            return -1;
        }
    }

    return 0;
}

static int net_ssl_endpoint_bio_read(void * ctx, unsigned char *out, size_t outlen) {
	if (!out) return 0;

    net_ssl_endpoint_t endpoint = ctx;
    if (endpoint == NULL) return 0;

    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    uint32_t length = net_endpoint_buf_size(endpoint->m_base_endpoint, net_ep_buf_read);
    if (length == 0) {
        /* If there's no data to read, say so. */
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    if (length > outlen) {
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ssl: %s: cli: bio: read: out-len=%d, buf-len=%d, read part",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                (int)outlen, length);
        }

        length = outlen;
    }

    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(endpoint->m_base_endpoint, net_ep_buf_read, length, &data) != 0) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: cli: bio: read: peak data fail, length=%d",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            length);
        return -1;
    }

    memcpy(out, data, length);
    
    net_endpoint_buf_consume(endpoint->m_base_endpoint, net_ep_buf_read, length);

    return length;
}

static int net_ssl_endpoint_bio_write(void * ctx, const unsigned char *in, size_t inlen) {
    net_ssl_endpoint_t endpoint = ctx;
    if (endpoint == NULL) return 0;

    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    uint32_t write_size = inlen;
    if (net_endpoint_buf_append(endpoint->m_base_endpoint, net_ep_buf_write, in, write_size) != 0) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: cli: bio: write: append buf fail, len=%d!",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), write_size);
        return -1;
    }

    return (int)write_size;
}

static void net_ssl_endpoint_ssl_fini(net_ssl_endpoint_t endpoint) {
    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    if (endpoint->m_ssl) {
        mbedtls_ssl_free(endpoint->m_ssl);
        mem_free(protocol->m_alloc, endpoint->m_ssl);
        endpoint->m_ssl = NULL;
    }

    if (endpoint->m_ssl_config) {
        mbedtls_ssl_config_free(endpoint->m_ssl_config);
        mem_free(protocol->m_alloc, endpoint->m_ssl_config);
        endpoint->m_ssl_config = NULL;
    }
}

static void net_ssl_endpoint_ssl_debug(void *ctx, int level, const char *file, int line, const char *str) {
    net_ssl_endpoint_t endpoint = ctx;

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        net_protocol_t base_protocol = net_endpoint_protocol(endpoint->m_base_endpoint);
        net_ssl_protocol_t protocol = net_protocol_data(base_protocol);

        protocol->m_em->m_curent_location.m_file = file;
        protocol->m_em->m_curent_location.m_line = line;
        
        const char * bk = strrchr(str, '\n');
        if (bk) {
            CPE_INFO(protocol->m_em, "|%d| %.*s", level, (int)(bk - str), str);
        }
        else {
            CPE_INFO(protocol->m_em, "|%d| %s", level, str);
        }

        protocol->m_em->m_curent_location.m_file = NULL;
        protocol->m_em->m_curent_location.m_line = 0;
    }
}

static int net_ssl_endpoint_ssl_init(net_ssl_endpoint_t endpoint) {
    net_protocol_t base_protocol = net_endpoint_protocol(endpoint->m_base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);
    int rv;

    assert(endpoint->m_ssl_config == NULL);
    assert(endpoint->m_ssl == NULL);
    assert(endpoint->m_runing_mode == net_ssl_endpoint_runing_mode_cli
           || endpoint->m_runing_mode == net_ssl_endpoint_runing_mode_svr);
    
    endpoint->m_ssl_config = mem_alloc(protocol->m_alloc, sizeof(mbedtls_ssl_config));
    if (endpoint->m_ssl_config == NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: init ssl: alloc client ssl_config fail",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        goto INIT_FAILED;
    }
    mbedtls_ssl_config_init(endpoint->m_ssl_config);
    
    if ((rv = mbedtls_ssl_config_defaults(
             endpoint->m_ssl_config,
             endpoint->m_runing_mode == net_ssl_endpoint_runing_mode_cli ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
             MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT))
        != 0)
    {
        char error_buf[1024];
        CPE_ERROR(
            protocol->m_em, "ssl: %s: init ssl: alloc client ssl_config_defaults fail, -0x%04X(%s)",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            -rv, net_ssl_strerror(error_buf, sizeof(error_buf), rv));
        goto INIT_FAILED;
    }
    mbedtls_ssl_conf_rng(endpoint->m_ssl_config, mbedtls_ctr_drbg_random, protocol->m_ctr_drbg);
    mbedtls_ssl_conf_dbg(endpoint->m_ssl_config, net_ssl_endpoint_ssl_debug, endpoint);
    //mbedtls_ssl_conf_max_version(endpoint->m_ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_4);

#if defined(MBEDTLS_SSL_RENEGOTIATION)
    mbedtls_ssl_conf_renegotiation(endpoint->m_ssl_config, MBEDTLS_SSL_RENEGOTIATION_ENABLED);
#endif

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    mbedtls_ssl_conf_session_tickets(endpoint->m_ssl_config, MBEDTLS_SSL_SESSION_TICKETS_DISABLED);
#endif
    
    if (endpoint->m_runing_mode == net_ssl_endpoint_runing_mode_cli) {
        mbedtls_ssl_conf_authmode(endpoint->m_ssl_config, MBEDTLS_SSL_VERIFY_NONE);

        if (protocol->m_ciphersuites) {
            mbedtls_ssl_conf_ciphersuites(endpoint->m_ssl_config, protocol->m_ciphersuites);
        }
    }

    if (endpoint->m_runing_mode == net_ssl_endpoint_runing_mode_svr) {
        mbedtls_ssl_conf_own_cert(endpoint->m_ssl_config, protocol->m_svr.m_cert, protocol->m_svr.m_pkey);
        mbedtls_ssl_conf_ciphersuites(endpoint->m_ssl_config, mbedtls_ssl_list_ciphersuites());
    }

    endpoint->m_ssl = mem_alloc(protocol->m_alloc, sizeof(mbedtls_ssl_context));
    if(endpoint->m_ssl == NULL) {
        CPE_ERROR(
            protocol->m_em, "ssl: %s: ssl init: alloc mbedtls_ssl_context fail",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        goto INIT_FAILED;
    }
    mbedtls_ssl_init(endpoint->m_ssl);

    if ((rv = mbedtls_ssl_setup(endpoint->m_ssl, endpoint->m_ssl_config)) != 0) {
        char error_buf[1024];
        CPE_ERROR(
            protocol->m_em, "ssl: %s: ssl init: mbedtls_ssl_setup fail, -0x%04X(%s)",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            -rv, net_ssl_strerror(error_buf, sizeof(error_buf), rv));
        goto INIT_FAILED;
    }

    mbedtls_ssl_set_bio(
        endpoint->m_ssl, endpoint,
        net_ssl_endpoint_bio_write, net_ssl_endpoint_bio_read, NULL);

    mbedtls_ssl_set_timer_cb(
        endpoint->m_ssl, &endpoint->m_timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);
    
    return 0;

INIT_FAILED:
    net_ssl_endpoint_ssl_fini(endpoint);
    return -1;
}

static int net_ssl_endpoint_ssl_verify(void *ctx, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
    net_ssl_endpoint_t endpoint = ctx;
    net_protocol_t base_protocol = net_endpoint_protocol(endpoint->m_base_endpoint);
    net_ssl_protocol_t protocol = net_protocol_data(base_protocol);
    int ret = 0;

    /*
     * If MBEDTLS_HAVE_TIME_DATE is defined, then the certificate date and time
     * validity checks will probably fail because this application does not set
     * up the clock correctly. We filter out date and time related failures
     * instead
     */
    *flags &= ~MBEDTLS_X509_BADCERT_FUTURE & ~MBEDTLS_X509_BADCERT_EXPIRED;

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        char gp_buf[1024];
        ret = mbedtls_x509_crt_info(gp_buf, sizeof(gp_buf), "\r  ", crt);
        if (ret < 0) {
            char error_buf[1024];
            CPE_ERROR(
                protocol->m_em, "ssl: %s: ssl verify: mbedtls_x509_crt_info() returned -0x%04X(%s)",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                -ret, net_ssl_strerror(error_buf, sizeof(error_buf), ret));
        } else {
            CPE_INFO(
                protocol->m_em, "ssl: %s: ssl verify: depth=%d\n%s",
                net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                depth, gp_buf);
        }
    }

    return ret;
}

int net_ssl_endpoint_set_state(net_ssl_endpoint_t endpoint, net_ssl_endpoint_state_t state) {
    if (endpoint->m_state == state) return 0;

    net_ssl_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    assert(endpoint->m_runing_mode != net_ssl_endpoint_runing_mode_init);
    
    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "ssl: %s: state: %s ==> %s",
            net_endpoint_dump(net_ssl_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ssl_endpoint_state_str(endpoint->m_state),
            net_ssl_endpoint_state_str(state));
    }

    endpoint->m_state = state;

    switch(state) {
    case net_ssl_endpoint_state_init:
        net_ssl_endpoint_ssl_fini(endpoint);
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
