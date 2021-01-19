#include "net_timer.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ebb_endpoint_i.h"
#include "net_ebb_request_i.h"

static net_ebb_request_t net_ebb_endpoint_new_request(void *data);
static void net_ebb_endpoint_timeout_cb(net_timer_t timer, void * ctx);
static void net_ebb_endpoint_close_cb(net_timer_t timer, void * ctx);

int net_ebb_endpoint_init(net_endpoint_t base_endpoint) {
    net_ebb_protocol_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_ebb_endpoint_t connection = net_endpoint_data(base_endpoint);

    connection->m_timer_timeout = net_timer_auto_create(net_endpoint_schedule(base_endpoint), net_ebb_endpoint_timeout_cb, connection);
    if (connection->m_timer_timeout == NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: create timeout timer fail", 
            net_endpoint_dump(net_ebb_protocol_tmp_buffer(service), base_endpoint));
        return -1;
    }
    CPE_ERROR(service->m_em, "xxxxx: create timer=%p", connection->m_timer_timeout);

    connection->m_timer_close = net_timer_auto_create(net_endpoint_schedule(base_endpoint), net_ebb_endpoint_close_cb, connection);
    if (connection->m_timer_close == NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: create close timer fail", 
            net_endpoint_dump(net_ebb_protocol_tmp_buffer(service), base_endpoint));
        net_timer_free(connection->m_timer_timeout);
        return -1;
    }
    
    net_ebb_request_parser_init(&connection->parser);
    connection->parser.data = connection;
    connection->parser.new_request = net_ebb_endpoint_new_request;

    TAILQ_INIT(&connection->m_requests);
    
    net_ebb_endpoint_timeout_reset(connection);

    return 0;
}

void net_ebb_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ebb_protocol_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_ebb_endpoint_t connection = net_endpoint_data(base_endpoint);

    while(!TAILQ_EMPTY(&connection->m_requests)) {
        net_ebb_request_free(TAILQ_FIRST(&connection->m_requests));
    }

    CPE_ERROR(service->m_em, "xxxxx: free timer=%p", connection->m_timer_timeout);
    if (connection->m_timer_timeout) {
        net_timer_free(connection->m_timer_timeout);
        connection->m_timer_timeout = NULL;
    }
    
    if (connection->m_timer_close) {
        net_timer_free(connection->m_timer_close);
        connection->m_timer_close = NULL;
    }
}

int net_ebb_endpoint_input(net_endpoint_t base_endpoint) {
    net_ebb_protocol_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_ebb_endpoint_t connection = net_endpoint_data(base_endpoint);

    net_ebb_endpoint_timeout_reset(connection);

    while(net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        uint32_t data_size = 0;
        void* data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_read, &data_size);
        if (data == NULL || data_size == 0) return 0;

        net_ebb_request_parser_execute(&connection->parser, data, data_size);

        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return -1;
        
        net_endpoint_buf_consume(base_endpoint, net_ep_buf_read, data_size);
        
        /* parse error? just drop the client. screw the 400 response */
        if (net_ebb_request_parser_has_error(&connection->parser)) {
            CPE_ERROR(
                service->m_em, "ebb: %s: request parser has error, auto close!",
                net_endpoint_dump(net_ebb_protocol_tmp_buffer(service), base_endpoint));
            return -1;
        }
    }

    return -1;
}

net_ebb_protocol_t net_ebb_endpoint_service(net_ebb_endpoint_t connection) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(connection);
    return net_protocol_data(net_endpoint_protocol(base_endpoint));
}

static net_ebb_request_t net_ebb_endpoint_new_request(void *data) {
    net_ebb_endpoint_t connection = data;
    return net_ebb_request_create(connection);;
}

static void net_ebb_endpoint_timeout_cb(net_timer_t timer, void * ctx) {
    net_ebb_endpoint_t connection = ctx;
    net_endpoint_t base_endpoint = net_endpoint_from_data(connection);
    net_ebb_protocol_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: timeout",
            net_endpoint_dump(net_ebb_protocol_tmp_buffer(service), base_endpoint));
    }

    net_endpoint_set_state(base_endpoint, net_endpoint_state_disable);
}

void net_ebb_endpoint_timeout_reset(net_ebb_endpoint_t connection) {
    net_ebb_protocol_t service = net_ebb_endpoint_service(connection);
    net_timer_active(connection->m_timer_timeout, service->m_cfg_connection_timeout_ms);
}

static void net_ebb_endpoint_close_cb(net_timer_t timer, void * ctx) {
    net_ebb_endpoint_t connection = ctx;
    net_endpoint_t base_endpoint = net_endpoint_from_data(connection);
    net_ebb_protocol_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (net_endpoint_protocol_debug(base_endpoint)) {
        CPE_INFO(
            service->m_em, "ebb: %s: do close",
            net_endpoint_dump(net_ebb_protocol_tmp_buffer(service), base_endpoint));
    }

    net_endpoint_set_state(base_endpoint, net_endpoint_state_disable);
}

void net_ebb_endpoint_schedule_close(net_ebb_endpoint_t connection) {
    net_timer_active(connection->m_timer_close, 0);
}

void net_ebb_endpoint_check_remove_done_requests(net_ebb_endpoint_t connection) {
    net_ebb_request_t request = TAILQ_FIRST(&connection->m_requests);

    while(request && request->m_state == net_ebb_request_state_complete) {
        net_ebb_request_free(request);

        request = TAILQ_FIRST(&connection->m_requests);
        if (request) {
            //TODO: copy data to ep
        }
    }
}
