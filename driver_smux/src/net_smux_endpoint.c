#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/error.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_smux_endpoint_i.h"
#include "net_smux_session_i.h"

void net_smux_endpoint_free(net_smux_endpoint_t endpoint) {
    net_endpoint_free(endpoint->m_base_endpoint);
}

net_smux_endpoint_t
net_smux_endpoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_smux_endpoint_init
        ? net_endpoint_protocol_data(base_endpoint)
        : NULL;
}

net_endpoint_t net_smux_endpoint_base_endpoint(net_smux_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

net_smux_session_t net_smux_endpoint_session(net_smux_endpoint_t endpoint) {
    return endpoint->m_session;
}

int net_smux_endpoint_init(net_endpoint_t base_endpoint) {
    net_smux_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_smux_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_runing_mode = net_smux_endpoint_runing_mode_init;
    endpoint->m_session = NULL;

    return 0;
}

void net_smux_endpoint_fini(net_endpoint_t base_endpoint) {
    net_smux_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_smux_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
}

int net_smux_endpoint_input(net_endpoint_t base_endpoint) {
    net_smux_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_smux_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    assert(net_endpoint_state(base_endpoint) != net_endpoint_state_deleting);
    if (endpoint->m_runing_mode == net_smux_endpoint_runing_mode_init) {
        CPE_ERROR(
            protocol->m_em, "smux: %s: input data in runing-mode %s",
            net_endpoint_dump(net_smux_protocol_tmp_buffer(protocol), base_endpoint),
            net_smux_endpoint_runing_mode_str(endpoint->m_runing_mode));
        return -1;
    }

    uint32_t data_len = net_endpoint_buf_size(base_endpoint, net_ep_buf_read);
    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_read, data_len, &data) != 0) {
        CPE_ERROR(
            protocol->m_em, "smux: %s: peak with size %d faild",
            net_endpoint_dump(net_smux_protocol_tmp_buffer(protocol), base_endpoint),
            data_len);
        return -1;
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 3) {
        CPE_INFO(
            protocol->m_em, "smux: %s: %s: net: <== %d bytes",
            net_endpoint_dump(net_smux_protocol_tmp_buffer(protocol), base_endpoint),
            net_smux_endpoint_runing_mode_str(endpoint->m_runing_mode),
            data_len);
    }

    return 0;
}

int net_smux_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    net_smux_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_smux_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    /* if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) { */
    /*     if (endpoint->m_runing_mode != net_smux_endpoint_runing_mode_init) { */
    /*         if (net_smux_endpoint_set_state(endpoint, net_smux_endpoint_state_setting) != 0) return -1; */
    /*     } */
    /* } */

    return 0;
}

net_smux_endpoint_runing_mode_t net_smux_endpoint_runing_mode(net_smux_endpoint_t endpoint) {
    return endpoint->m_runing_mode;
}

void net_smux_session_set_session(net_smux_endpoint_t endpoint, net_smux_session_t session) {
    if (endpoint->m_session == session) return;

    if (endpoint->m_session) {
        assert(endpoint->m_session->m_underline.m_type == net_smux_session_underline_tcp);
        assert(endpoint->m_session->m_underline.m_tcp.m_endpoint == endpoint);
        endpoint->m_session->m_underline.m_tcp.m_endpoint = NULL;
    }

    endpoint->m_session = session;

    if (endpoint->m_session) {
        assert(endpoint->m_session->m_underline.m_type == net_smux_session_underline_tcp);
        assert(endpoint->m_session->m_underline.m_tcp.m_endpoint == NULL);
        endpoint->m_session->m_underline.m_tcp.m_endpoint = endpoint;
    }
}

const char * net_smux_endpoint_runing_mode_str(net_smux_endpoint_runing_mode_t runing_mode) {
    switch(runing_mode) {
    case net_smux_endpoint_runing_mode_init:
        return "init";
    case net_smux_endpoint_runing_mode_cli:
        return "cli";
    case net_smux_endpoint_runing_mode_svr:
        return "svr";
    }
}
