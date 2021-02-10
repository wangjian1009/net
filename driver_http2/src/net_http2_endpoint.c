#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/error.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_http2_endpoint_i.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_stream_group_i.h"
#include "net_http2_stream_acceptor_i.h"
#include "net_http2_utils.h"

extern struct wslay_event_callbacks s_net_http2_endpoint_callbacks;
void net_http2_endpoint_do_flush(net_http2_endpoint_t endpoint);
void net_http2_endpoint_delay_process(net_timer_t timer, void * ctx);
void net_http2_endpoint_sync_stream_state(net_http2_endpoint_t endpoint);

void net_http2_endpoint_free(net_http2_endpoint_t endpoint) {
    net_endpoint_free(endpoint->m_base_endpoint);
}

net_http2_endpoint_t
net_http2_endpoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_http2_endpoint_init
        ? net_endpoint_protocol_data(base_endpoint)
        : NULL;
}

net_endpoint_t net_http2_endpoint_base_endpoint(net_http2_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

int net_http2_endpoint_init(net_endpoint_t base_endpoint) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_runing_mode = net_http2_endpoint_runing_mode_init;
    endpoint->m_state = net_http2_endpoint_state_init;
    endpoint->m_remote_settings.m_stream_capacity = 0;
    endpoint->m_in_processing = 0;
    endpoint->m_delay_processor = NULL;
    endpoint->m_http2_session = NULL;
    TAILQ_INIT(&endpoint->m_streams);
    TAILQ_INIT(&endpoint->m_sending_streams);
    
    endpoint->m_delay_processor =
        net_timer_auto_create(
            net_endpoint_schedule(base_endpoint), net_http2_endpoint_delay_process, endpoint);
    if (endpoint->m_delay_processor == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: init: create delay processor fail",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    return 0;
}

void net_http2_endpoint_fini(net_endpoint_t base_endpoint) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    while(!TAILQ_EMPTY(&endpoint->m_streams)) {
        net_http2_stream_endpoint_t stream = TAILQ_FIRST(&endpoint->m_streams);
        net_http2_stream_endpoint_set_control(stream, NULL);
    }
    assert(TAILQ_EMPTY(&endpoint->m_sending_streams));
    
    if (endpoint->m_http2_session) {
        nghttp2_session_del(endpoint->m_http2_session);
        endpoint->m_http2_session = NULL;
    }

    if (endpoint->m_delay_processor) {
        net_timer_free(endpoint->m_delay_processor);
        endpoint->m_delay_processor = NULL;
    }
}

int net_http2_endpoint_input(net_endpoint_t base_endpoint) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    assert(net_endpoint_state(base_endpoint) != net_endpoint_state_deleting);

    uint32_t data_len = net_endpoint_buf_size(base_endpoint, net_ep_buf_read);
    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_read, data_len, &data) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: peak with size %d faild",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), base_endpoint),
            data_len);
        return -1;
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 3) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: net: <== %d bytes",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            data_len);
    }

    assert(!endpoint->m_in_processing);
    endpoint->m_in_processing = 1;
    
    assert(endpoint->m_http2_session);
    ssize_t readlen = nghttp2_session_mem_recv(endpoint->m_http2_session, data, data_len);
    if (readlen < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2 session input fail: %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            nghttp2_strerror((int)readlen));
        endpoint->m_in_processing = 0;
        return -1;
    }

    if (net_endpoint_is_active(base_endpoint)) {
        net_endpoint_buf_consume(endpoint->m_base_endpoint, net_ep_buf_read, (size_t)readlen);
        net_http2_endpoint_do_flush(endpoint);
    }
    
    endpoint->m_in_processing = 0;
    return 0;
}

void net_http2_endpoint_sync_stream_state(net_http2_endpoint_t endpoint) {
    net_http2_stream_endpoint_t stream, next_stream;

    for(stream = TAILQ_FIRST(&endpoint->m_streams); stream; stream = next_stream) {
        next_stream = TAILQ_NEXT(stream, m_next_for_control);

        if (net_http2_stream_endpoint_sync_state(stream) != 0) {
            net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
        }
    }
}

int net_http2_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        if (endpoint->m_runing_mode != net_http2_endpoint_runing_mode_init) {
            if (net_http2_endpoint_set_state(endpoint, net_http2_endpoint_state_setting) != 0) return -1;
        }
    }

    net_http2_endpoint_sync_stream_state(endpoint);

    return 0;
}

net_http2_endpoint_runing_mode_t net_http2_endpoint_runing_mode(net_http2_endpoint_t endpoint) {
    return endpoint->m_runing_mode;
}

net_http2_endpoint_state_t net_http2_endpoint_state(net_http2_endpoint_t endpoint) {
    return endpoint->m_state;
}

void net_http2_endpoint_set_stream_group(
    net_http2_endpoint_t endpoint, net_http2_stream_group_t stream_group)
{
    assert(endpoint->m_runing_mode == net_http2_endpoint_runing_mode_cli);
    
    if (endpoint->m_cli.m_stream_group == stream_group) return;

    if (endpoint->m_cli.m_stream_group) {
        TAILQ_REMOVE(&endpoint->m_cli.m_stream_group->m_endpoints, endpoint, m_cli.m_next_for_group);
        endpoint->m_cli.m_stream_group = NULL;
    }

    endpoint->m_cli.m_stream_group = stream_group;

    if (endpoint->m_cli.m_stream_group) {
        TAILQ_INSERT_TAIL(&endpoint->m_cli.m_stream_group->m_endpoints, endpoint, m_cli.m_next_for_group);
    }
}

void net_http2_endpoint_set_stream_acceptor(
    net_http2_endpoint_t endpoint, net_http2_stream_acceptor_t stream_acceptor)
{
    assert(endpoint->m_runing_mode == net_http2_endpoint_runing_mode_svr);
    
    if (endpoint->m_svr.m_stream_acceptor == stream_acceptor) return;

    if (endpoint->m_svr.m_stream_acceptor) {
        TAILQ_REMOVE(&endpoint->m_svr.m_stream_acceptor->m_endpoints, endpoint, m_svr.m_next_for_acceptor);
        endpoint->m_svr.m_stream_acceptor = NULL;
    }

    endpoint->m_svr.m_stream_acceptor = stream_acceptor;

    if (endpoint->m_svr.m_stream_acceptor) {
        TAILQ_INSERT_TAIL(&endpoint->m_svr.m_stream_acceptor->m_endpoints, endpoint, m_svr.m_next_for_acceptor);
    }
}

void net_http2_endpoint_do_flush(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    assert(endpoint->m_in_processing);
    if (endpoint->m_http2_session == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: flush: no http2 session",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        return;
    }
    
    do {
        while(!TAILQ_EMPTY(&endpoint->m_sending_streams)) {
            net_http2_stream_endpoint_t stream = TAILQ_FIRST(&endpoint->m_sending_streams);
            if (net_http2_stream_endpoint_delay_send_data(stream) != 0) {
                if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_error) != 0) {
                    net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
                }
            }

            if (endpoint->m_http2_session == NULL) return;
        }

        if (endpoint->m_http2_session == NULL) return;

        int rv = nghttp2_session_send(endpoint->m_http2_session);
        if (rv != 0) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: flush: session send failed: %s",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                nghttp2_strerror(rv));
            //net_http2_endpoint_pyhsical_close(endpoint, net_endpoint_network_errno_logic, nghttp2_strerror(rv));
            return;
        }
    } while (!TAILQ_EMPTY(&endpoint->m_sending_streams));
}

void net_http2_endpoint_schedule_flush(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (endpoint->m_in_processing) return;

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "http2: %s: flush scheduled",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
    }

    net_http2_endpoint_schedule_delay_processor(endpoint);
}

void net_http2_endpoint_delay_flush(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "http2: %s: delay flush begin",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
    }

    assert(!endpoint->m_in_processing);
    endpoint->m_in_processing = 1;
    
    net_http2_endpoint_do_flush(endpoint);

    endpoint->m_in_processing = 0;
}

void net_http2_endpoint_schedule_delay_processor(net_http2_endpoint_t endpoint) {
    net_timer_active(endpoint->m_delay_processor, 0);
}

void net_http2_endpoint_cancel_delay_processor(net_http2_endpoint_t endpoint) {
    net_timer_cancel(endpoint->m_delay_processor);
}

void net_http2_endpoint_delay_process(net_timer_t timer, void * ctx) {
    net_http2_endpoint_t endpoint = ctx;
    net_http2_endpoint_delay_flush(endpoint);
}

int net_http2_endpoint_set_state(net_http2_endpoint_t endpoint, net_http2_endpoint_state_t state) {
    if (endpoint->m_state == state) return 0;

    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    assert(endpoint->m_runing_mode != net_http2_endpoint_runing_mode_init);
    
    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2-state: %s ==> %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            net_http2_endpoint_state_str(endpoint->m_state),
            net_http2_endpoint_state_str(state));
    }

    endpoint->m_state = state;

    switch(state) {
    case net_http2_endpoint_state_init:
        break;
    case net_http2_endpoint_state_setting:
        if (net_http2_endpoint_http2_send_settings(endpoint) != 0) return -1;
        net_http2_endpoint_schedule_flush(endpoint);
        break;
    case net_http2_endpoint_state_streaming:
        net_http2_endpoint_sync_stream_state(endpoint);
        break;
    }

    return 0;
}

int net_http2_endpoint_set_runing_mode(net_http2_endpoint_t endpoint, net_http2_endpoint_runing_mode_t runing_mode) {
    if (endpoint->m_runing_mode == runing_mode) return 0;

    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: runing-mode: %s ==> %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            net_http2_endpoint_runing_mode_str(runing_mode));
    }

    switch(endpoint->m_runing_mode) {
    case net_http2_endpoint_runing_mode_init:
        break;
    case net_http2_endpoint_runing_mode_cli:
        if (endpoint->m_cli.m_stream_group) {
            net_http2_endpoint_set_stream_group(endpoint, NULL);
            assert(endpoint->m_cli.m_stream_group == NULL);
        }

        if (endpoint->m_http2_session) {
            nghttp2_session_del(endpoint->m_http2_session);
            endpoint->m_http2_session = NULL;
        }
        break;
    case net_http2_endpoint_runing_mode_svr:
        if (endpoint->m_svr.m_stream_acceptor) {
            net_http2_endpoint_set_stream_acceptor(endpoint, NULL);
            assert(endpoint->m_svr.m_stream_acceptor == NULL);
        }

        if (endpoint->m_http2_session) {
            nghttp2_session_del(endpoint->m_http2_session);
            endpoint->m_http2_session = NULL;
        }
        break;
    }
    
    endpoint->m_runing_mode = runing_mode;

    switch(endpoint->m_runing_mode) {
    case net_http2_endpoint_runing_mode_init:
        break;
    case net_http2_endpoint_runing_mode_cli:
        endpoint->m_cli.m_stream_group = NULL;
        assert(endpoint->m_http2_session == NULL);
        if (nghttp2_session_client_new(&endpoint->m_http2_session, protocol->m_http2_callbacks, endpoint) != 0) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: cli: nghttp2_session_client_new error",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
            return -1;
        }

        if (net_endpoint_state(endpoint->m_base_endpoint) == net_endpoint_state_established) {
            if (net_http2_endpoint_set_state(endpoint, net_http2_endpoint_state_setting) != 0) return -1;
        }

        break;
    case net_http2_endpoint_runing_mode_svr:
        endpoint->m_svr.m_stream_acceptor = NULL;
        assert(endpoint->m_http2_session == NULL);
        if (nghttp2_session_server_new(&endpoint->m_http2_session, protocol->m_http2_callbacks, endpoint) != 0) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: svr: nghttp2_session_server_new error",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
            return -1;
        }

        if (net_endpoint_state(endpoint->m_base_endpoint) == net_endpoint_state_established) {
            if (net_http2_endpoint_set_state(endpoint, net_http2_endpoint_state_setting) != 0) return -1;
        }

        break;
    }

    return 0;
}

const char * net_http2_endpoint_state_str(net_http2_endpoint_state_t state) {
    switch(state) {
    case net_http2_endpoint_state_init:
        return "init";
    case net_http2_endpoint_state_setting:
        return "setting";
    case net_http2_endpoint_state_streaming:
        return "streaming";
    }
}

const char * net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode_t runing_mode) {
    switch(runing_mode) {
    case net_http2_endpoint_runing_mode_init:
        return "init";
    case net_http2_endpoint_runing_mode_cli:
        return "cli";
    case net_http2_endpoint_runing_mode_svr:
        return "svr";
    }
}
