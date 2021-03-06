#include "net_timer.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_http_svr_endpoint_i.h"
#include "net_http_svr_request_i.h"

static net_http_svr_request_t net_http_svr_endpoint_new_request(void *data);
static void net_http_svr_endpoint_timeout_cb(net_timer_t timer, void * ctx);
static void net_http_svr_endpoint_close_cb(net_timer_t timer, void * ctx);

int net_http_svr_endpoint_init(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);

    connection->m_timer_timeout =
        net_timer_auto_create(schedule, net_http_svr_endpoint_timeout_cb, base_endpoint);
    if (connection->m_timer_timeout == NULL) {
        CPE_ERROR(
            service->m_em, "http_svr: %s: create timeout timer fail", 
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint));
        return -1;
    }

    connection->m_timer_close =
        net_timer_auto_create(schedule, net_http_svr_endpoint_close_cb, base_endpoint);
    if (connection->m_timer_close == NULL) {
        CPE_ERROR(
            service->m_em, "http_svr: %s: create close timer fail", 
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint));
        net_timer_free(connection->m_timer_timeout);
        return -1;
    }
    
    net_http_svr_request_parser_init(&connection->parser);
    connection->parser.data = base_endpoint;
    connection->parser.new_request = net_http_svr_endpoint_new_request;

    TAILQ_INIT(&connection->m_requests);
    
    net_http_svr_endpoint_timeout_reset(base_endpoint);

    return 0;
}

void net_http_svr_endpoint_fini(net_endpoint_t base_endpoint) {
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);

    while(!TAILQ_EMPTY(&connection->m_requests)) {
        net_http_svr_request_t request = TAILQ_FIRST(&connection->m_requests);
        if (request->m_is_processing) {
            net_http_svr_request_clear_endpoint(request);
        }
        else {
            net_http_svr_request_free(request);
        }
    }

    if (connection->m_timer_timeout) {
        net_timer_free(connection->m_timer_timeout);
        connection->m_timer_timeout = NULL;
    }
    
    if (connection->m_timer_close) {
        net_timer_free(connection->m_timer_close);
        connection->m_timer_close = NULL;
    }
}

int net_http_svr_endpoint_input(net_endpoint_t base_endpoint) {
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);

    net_http_svr_endpoint_timeout_reset(base_endpoint);

    while(net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        uint32_t data_size = 0;
        void* data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_read, &data_size);
        if (data == NULL || data_size == 0) return 0;

        net_http_svr_request_parser_execute(&connection->parser, data, data_size);

        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return -1;
        
        net_endpoint_buf_consume(base_endpoint, net_ep_buf_read, data_size);
        
        /* parse error? just drop the client. screw the 400 response */
        if (net_http_svr_request_parser_has_error(&connection->parser)) {
            CPE_ERROR(
                service->m_em, "http_svr: %s: request parser has error, auto close!",
                net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint));
            return -1;
        }
    }

    return -1;
}

net_http_svr_protocol_t net_http_svr_endpoint_service(net_endpoint_t base_endpoint) {
    return net_protocol_data(net_endpoint_protocol(base_endpoint));
}

net_http_svr_endpoint_t net_http_svr_endpoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_http_svr_endpoint_init
        ? net_endpoint_data(base_endpoint)
        : NULL;
}

static net_http_svr_request_t net_http_svr_endpoint_new_request(void *data) {
    net_endpoint_t base_endpoint = data;
    return net_http_svr_request_create(base_endpoint);
}

static void net_http_svr_endpoint_timeout_cb(net_timer_t timer, void * ctx) {
    net_endpoint_t base_endpoint = ctx;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            service->m_em, "http_svr: %s: timeout",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint));
    }

    net_endpoint_set_state(base_endpoint, net_endpoint_state_disable);
}

void net_http_svr_endpoint_timeout_reset(net_endpoint_t base_endpoint) {
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);
    net_timer_active(connection->m_timer_timeout, service->m_cfg_connection_timeout_ms);
}

static void net_http_svr_endpoint_close_cb(net_timer_t timer, void * ctx) {
    net_endpoint_t base_endpoint = ctx;
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    net_http_svr_protocol_t service = net_http_svr_endpoint_service(base_endpoint);

    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            service->m_em, "http_svr: %s: do close",
            net_endpoint_dump(net_http_svr_protocol_tmp_buffer(service), base_endpoint));
    }

    net_endpoint_set_close_after_send(base_endpoint);
}

void net_http_svr_endpoint_schedule_close(net_endpoint_t base_endpoint) {
    net_http_svr_endpoint_t connection = net_endpoint_protocol_data(base_endpoint);
    if (connection->m_timer_close) {
        net_timer_active(connection->m_timer_close, 0);
    }
}

void net_http_svr_endpoint_check_remove_done_requests(net_http_svr_endpoint_t connection) {
    net_http_svr_request_t request = TAILQ_FIRST(&connection->m_requests);

    while(request && request->m_state == net_http_svr_request_state_complete) {
        net_http_svr_request_free(request);

        request = TAILQ_FIRST(&connection->m_requests);
        if (request) {
            //TODO: copy data to ep
        }
    }
}
