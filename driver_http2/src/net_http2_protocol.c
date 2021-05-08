#include <assert.h>
#include "cpe/pal/pal_stdio.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_http2_protocol_i.h"
#include "net_http2_endpoint_i.h"

void net_http2_protocol_fini(net_protocol_t base_protocol);

int net_http2_protocol_init(net_protocol_t base_protocol) {
    net_http2_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = NULL;
    protocol->m_em = NULL;
    protocol->m_cfg_idle_timeout_ms = 5 * 60 * 1000;
    protocol->m_http2_callbacks = NULL;
    protocol->m_http2_options = NULL;
    protocol->m_max_req_id = 0;
    mem_buffer_init(&protocol->m_data_buffer, NULL);

    if (nghttp2_option_new(&protocol->m_http2_options) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: nghttp2_session_callbacks_new error",
            net_protocol_name(base_protocol));
        net_http2_protocol_fini(base_protocol);
        return -1;
    }
    assert(protocol->m_http2_options != NULL);
    
    if (nghttp2_session_callbacks_new(&protocol->m_http2_callbacks) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: nghttp2_session_callbacks_new error",
            net_protocol_name(base_protocol));
        net_http2_protocol_fini(base_protocol);
        return -1;
    }
    assert(protocol->m_http2_callbacks != NULL);

    nghttp2_session_callbacks_set_send_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_send_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_frame_send_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_on_frame_send_callback);
    nghttp2_session_callbacks_set_on_frame_not_send_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_on_frame_not_send_callback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_on_data_chunk_recv_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_on_stream_close_callback);
    nghttp2_session_callbacks_set_on_header_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_on_header_callback);
    nghttp2_session_callbacks_set_on_invalid_header_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_on_invalid_header_callback);
    nghttp2_session_callbacks_set_on_begin_headers_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_on_begin_headers_callback);
    nghttp2_session_callbacks_set_error_callback2(
        protocol->m_http2_callbacks, net_http2_endpoint_on_error_callback);
    nghttp2_session_callbacks_set_send_data_callback(
        protocol->m_http2_callbacks, net_http2_endpoint_send_data_callback);

    return 0;
}

void net_http2_protocol_fini(net_protocol_t base_protocol) {
    net_http2_protocol_t protocol = net_protocol_data(base_protocol);

    if (protocol->m_http2_options) {
        nghttp2_option_del(protocol->m_http2_options);
        protocol->m_http2_options = NULL;
    }
    
    if (protocol->m_http2_callbacks) {
        nghttp2_session_callbacks_del(protocol->m_http2_callbacks);
        protocol->m_http2_callbacks = NULL;
    }

    mem_buffer_clear(&protocol->m_data_buffer);
}

net_http2_protocol_t
net_http2_protocol_create(
    net_schedule_t schedule, const char * addition_name,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "http2-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "http2");
    }

    net_protocol_t base_protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_http2_protocol),
            net_http2_protocol_init,
            net_http2_protocol_fini,
            /*endpoint*/
            sizeof(struct net_http2_endpoint),
            net_http2_endpoint_init,
            net_http2_endpoint_fini,
            net_http2_endpoint_input,
            net_http2_endpoint_on_state_change,
            net_http2_endpoint_calc_size,
            NULL);

    net_http2_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_alloc = alloc;
    protocol->m_em = em;
    return protocol;
}

void net_http2_protocol_free(net_http2_protocol_t protocol) {
    net_protocol_free(net_protocol_from_data(protocol));
}

net_http2_protocol_t
net_http2_protocol_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "http2-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "http2");
    }

    net_protocol_t protocol = net_protocol_find(schedule, name);
    return protocol ? net_http2_protocol_cast(protocol) : NULL;
}

net_http2_protocol_t
net_http2_protocol_cast(net_protocol_t base_protocol) {
    return net_protocol_init_fun(base_protocol) == net_http2_protocol_init
        ? net_protocol_data(base_protocol)
        : NULL;
}

void net_http2_protocol_set_no_http_messaging(net_http2_protocol_t protocol, uint8_t val) {
    nghttp2_option_set_no_http_messaging(protocol->m_http2_options, val);
}

mem_buffer_t net_http2_protocol_tmp_buffer(net_http2_protocol_t protocol) {
    return net_schedule_tmp_buffer(net_protocol_schedule(net_protocol_from_data(protocol)));
}
