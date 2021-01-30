#include <assert.h>
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ws_svr_endpoint_i.h"
#include "net_ws_svr_stream_endpoint_i.h"

#define net_ws_svr_endpoint_ep_write_cache net_ep_buf_user1
#define net_ws_svr_endpoint_ep_read_cache net_ep_buf_user2

static void net_ws_svr_endpoint_dump_error(net_endpoint_t base_endpoint, int val);

static int net_ws_svr_endpoint_do_handshake(net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint);

static int net_ws_svr_endpoint_update_error(net_endpoint_t base_endpoint, int err, int r);

net_ws_svr_endpoint_t net_ws_svr_endpoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_ws_svr_endpoint_init
        ? net_endpoint_protocol_data(base_endpoint)
        : NULL;
}

net_endpoint_t net_ws_svr_endpoint_stream(net_endpoint_t base_endpoint) {
    net_ws_svr_endpoint_t endpoint = net_ws_svr_endpoint_cast(base_endpoint);
    if (endpoint == NULL) return NULL;
    if (endpoint->m_stream == NULL) return NULL;
    return net_endpoint_from_data(endpoint->m_stream);
}

int net_ws_svr_endpoint_init(net_endpoint_t base_endpoint) {
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_stream = NULL;
    endpoint->m_state = net_ws_svr_endpoint_ws_handshake;

    return 0;
}

void net_ws_svr_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);

    if (endpoint->m_stream) {
        assert(endpoint->m_stream->m_underline == base_endpoint);
        endpoint->m_stream->m_underline = NULL;
        endpoint->m_stream = NULL;
    }
}

int net_ws_svr_endpoint_input(net_endpoint_t base_endpoint) {
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_state == net_ws_svr_endpoint_ws_handshake) {
        if (net_ws_svr_endpoint_do_handshake(base_endpoint, endpoint) != 0) return -1;
    }

READ_AGAIN:    
    if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return -1;

    /* uint32_t input_data_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_read); */
    /* if (input_data_size == 0) return 0; */

    /* void * data = net_endpoint_buf_alloc_at_least(base_endpoint, &input_data_size); */
    /* if (data == NULL) { */
    /*     CPE_ERROR( */
    /*         protocol->m_em, "net: ws: %s: input: alloc data for read fail", */
    /*         net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint)); */
    /*     return -1; */
    /* } */

    /* ERR_clear_error(); */
    /* int r = SSL_read(endpoint->m_ws, data, input_data_size); */
    /* if (r > 0) { */
    /*     if (net_endpoint_buf_supply(base_endpoint, net_ws_svr_endpoint_ep_read_cache, r) != 0) { */
    /*         CPE_ERROR( */
    /*             protocol->m_em, "net: ws: %s: input: load data to input cache fail", */
    /*             net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint)); */
    /*         return -1; */
    /*     } */

    /*     net_endpoint_t base_endpoint = */
    /*         endpoint->m_ws_endpoint ? net_endpoint_from_data(endpoint->m_ws_endpoint) : NULL; */

    /*     if (base_endpoint */
    /*         && net_endpoint_state(base_endpoint) == net_endpoint_state_established) */
    /*     { */
    /*         if (net_endpoint_buf_append_from_other( */
    /*                 base_endpoint, net_ep_buf_read, */
    /*                 base_endpoint, net_ws_svr_endpoint_ep_read_cache, 0) != 0) */
    /*         { */
    /*             CPE_ERROR( */
    /*                 protocol->m_em, "net: ws: %s: input: forward data to ws ep fail", */
    /*                 net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint)); */
    /*             return -1; */
    /*         } */
    /*         else { */
    /*             goto READ_AGAIN; */
    /*         } */
    /*     } */
    /*     else { */
    /*         net_endpoint_buf_clear(base_endpoint, net_ws_svr_endpoint_ep_read_cache); */
    /*         CPE_ERROR( */
    /*             protocol->m_em, "net: ws: %s: input: no established ws endpoint(state=%s), clear cached data", */
    /*             net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint), */
    /*             net_endpoint_state_str(net_endpoint_state(base_endpoint))); */
    /*     } */
    /* } */
    /* else { */
    /*     net_endpoint_buf_release(base_endpoint); */

    /*     int err = SSL_get_error(endpoint->m_ws, r); */
    /*     return net_ws_svr_endpoint_update_error(base_endpoint, err, r); */
    /* } */
    
    return 0;
}

int net_ws_svr_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_endpoint_t base_stream = endpoint->m_stream ? net_endpoint_from_data(endpoint->m_stream) : NULL;
    
    switch (net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
    case net_endpoint_state_deleting:
        return 0;
    case net_endpoint_state_established:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) {
                net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_error:
        if (base_stream) {
            net_endpoint_set_error(
                base_stream, net_endpoint_error_source(base_stream),
                net_endpoint_error_no(base_stream), net_endpoint_error_msg(base_stream));

            if (net_endpoint_set_state(base_stream, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_disable:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
                return 0;
            }
        }
        return 0;
    case net_endpoint_state_read_closed:
        return 0;
    case net_endpoint_state_write_closed:
        return 0;
    }
}

int net_ws_svr_endpoint_write(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf)
{
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    
    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        if (net_endpoint_protocol_debug(base_endpoint)) {
            CPE_INFO(
                protocol->m_em, "net: ws: %s: write: cache %d data in state %s!",
                net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint),
                net_endpoint_buf_size(from_ep, from_buf),
                net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        }

        if (base_endpoint == from_ep) {
            return net_endpoint_buf_append_from_self(
                base_endpoint, net_ws_svr_endpoint_ep_write_cache, from_buf, 0);
        }
        else {
            return net_endpoint_buf_append_from_other(
                base_endpoint, net_ws_svr_endpoint_ep_write_cache, from_ep, from_buf, 0);
        }
    case net_endpoint_state_established:
        switch(endpoint->m_state) {
        case net_ws_svr_endpoint_ws_handshake:
            if (net_endpoint_protocol_debug(base_endpoint)) {
                CPE_INFO(
                    protocol->m_em, "net: ws: %s: write: cache %d data in state %s.handshake!",
                    net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint),
                    net_endpoint_buf_size(from_ep, from_buf),
                    net_endpoint_state_str(net_endpoint_state(base_endpoint)));
            }

            if (base_endpoint == from_ep) {
                return net_endpoint_buf_append_from_self(
                    base_endpoint, net_ws_svr_endpoint_ep_write_cache, from_buf, 0);
            }
            else {
                return net_endpoint_buf_append_from_other(
                    base_endpoint, net_ws_svr_endpoint_ep_write_cache, from_ep, from_buf, 0);
            }
        case net_ws_svr_endpoint_ws_open:
            break;
        }
        break;
    case net_endpoint_state_error:
    case net_endpoint_state_deleting:
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: write: can`t write in state %s!",
            net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint),
            net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        return -1;
    }

    uint32_t data_size = net_endpoint_buf_size(from_ep, from_buf);
    if (data_size == 0) return 0;

    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(from_ep, from_buf, data_size, &data) != 0) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: write: peak data fail!",
            net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    /* int r = SSL_write(endpoint->m_ws, data, data_size); */
    /* if (r < 0) { */
    /*     int err = SSL_get_error(endpoint->m_ws, r); */
    /*     return net_ws_svr_endpoint_update_error(base_endpoint, err, r); */
    /* } */
    
    /* net_endpoint_buf_consume(from_ep, from_buf, r); */

    /* if (net_endpoint_protocol_debug(base_endpoint)) { */
    /*     CPE_INFO( */
    /*         protocol->m_em, "net: ws: %s: ==> %d data!", */
    /*         net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint), r); */
    /* } */
    
    return 0;
}

int net_ws_svr_endpoint_do_handshake(net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint) {
    return 0;
}
