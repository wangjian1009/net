#include "net_timer.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ebb_connection_i.h"

static net_ebb_request_t net_ebb_connection_new_request(void *data);
static void net_ebb_connection_timeout(net_timer_t timer, void * ctx);
static void net_ebb_connection_reset_timeout(net_ebb_service_t service, net_ebb_connection_t connection);

int net_ebb_connection_init(net_endpoint_t endpoint) {
    net_ebb_service_t service = net_protocol_data(net_endpoint_protocol(endpoint));
    net_ebb_connection_t connection = net_endpoint_data(endpoint);

    connection->m_endpoint = endpoint;

    connection->m_timeout = net_timer_auto_create(net_endpoint_schedule(endpoint), net_ebb_connection_timeout, connection);
    if (connection->m_timeout == NULL) {
        CPE_ERROR(
            service->m_em, "ebb: %s: create timer fail", 
            net_endpoint_dump(net_ebb_service_tmp_buffer(service), endpoint));
        return -1;
    }
    
    net_ebb_request_parser_init(&connection->parser);
    connection->parser.data = connection;
    connection->parser.new_request = net_ebb_connection_new_request;

    connection->new_request = NULL;
    connection->on_timeout = NULL;
    connection->on_close = NULL;
    net_ebb_connection_reset_timeout(service, connection);

    return 0;
}

void net_ebb_connection_fini(net_endpoint_t endpoint) {
    net_ebb_service_t service = net_protocol_data(net_endpoint_protocol(endpoint));
    net_ebb_connection_t connection = net_endpoint_data(endpoint);
    
    net_timer_free(connection->m_timeout);
    connection->m_timeout = NULL;
}

int net_ebb_connection_input(net_endpoint_t endpoint) {
    net_ebb_service_t service = net_protocol_data(net_endpoint_protocol(endpoint));
    net_ebb_connection_t connection = net_endpoint_data(endpoint);

    net_ebb_connection_reset_timeout(service, connection);

    do {
        uint32_t data_size = 0;
        void* data = net_endpoint_buf_peak(endpoint, net_ep_buf_read, &data_size);
        if (data == NULL) return 0;

        net_ebb_request_parser_execute(&connection->parser, data, data_size);

        /* parse error? just drop the client. screw the 400 response */
        if (net_ebb_request_parser_has_error(&connection->parser)) {
            CPE_ERROR(
                service->m_em, "ebb: %s: request parser has error, auto close!",
                net_endpoint_dump(net_ebb_service_tmp_buffer(service), endpoint));
            return -1;
        }
    } while (1);
}

int net_ebb_connection_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state) {
    //net_ebb_connection_reset_timeout(connection);

    //net_ebb_request_parser_execute(&connection->parser, recv_buffer, recved);

    /* parse error? just drop the client. screw the 400 response */
    //if (net_ebb_request_parser_has_error(&connection->parser)) goto error;
    
    return -1;
}

static net_ebb_request_t net_ebb_connection_new_request(void *data) {
    net_ebb_connection_t connection = data;
    if(connection->new_request)
        return connection->new_request(connection);
    return NULL;
}

static void net_ebb_connection_timeout(net_timer_t timer, void * ctx) {
    net_ebb_connection_t connection = ctx;
    net_endpoint_t base_endpoint = net_endpoint_from_data(connection);
    net_ebb_service_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));

    //printf("on_timeout\n");

    /* if on_timeout returns true, we don't time out */
    if (connection->on_timeout) {
        int r = connection->on_timeout(connection);

        if (r == EBB_AGAIN) {
            net_ebb_connection_reset_timeout(service, connection);
            return;
        }
    }

    net_endpoint_close_after_send(connection->m_endpoint);
}

void net_ebb_connection_reset_timeout(net_ebb_service_t service, net_ebb_connection_t connection) {
    net_timer_active(connection->m_timeout, service->m_cfg_connection_timeout_ms);
}
