#include "net_timer.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ebb_connection_i.h"

static net_ebb_request_t net_ebb_connection_new_request(void *data);
static void net_ebb_connection_timeout(net_timer_t timer, void * ctx);
static void net_ebb_connection_reset_timeout(net_ebb_connection_t connection);

int net_ebb_connection_init(net_endpoint_t endpoint) {
    net_ebb_service_t service = net_protocol_data(net_endpoint_protocol(endpoint));
    net_ebb_connection_t connection = net_endpoint_data(endpoint);

    connection->m_endpoint = endpoint;

    connection->m_timeout = net_timer_auto_create(net_endpoint_schedule(endpoint), net_ebb_connection_timeout, connection);
    if (connection->m_timeout == NULL) {
        //CPE_ERROR();
        return -1;
    }

    net_ebb_request_parser_init(&connection->parser);
    connection->parser.data = connection;
    connection->parser.new_request = net_ebb_connection_new_request;

    connection->new_request = NULL;
    connection->on_timeout = NULL;
    connection->on_close = NULL;

    return 0;
}

void net_ebb_connection_fini(net_endpoint_t endpoint) {
}

int net_ebb_connection_input(net_endpoint_t endpoint) {
    return 0;
}

int net_ebb_connection_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state) {
    //net_ebb_connection_reset_timeout(connection);

    //net_ebb_request_parser_execute(&connection->parser, recv_buffer, recved);

    /* parse error? just drop the client. screw the 400 response */
    //if (net_ebb_request_parser_has_error(&connection->parser)) goto error;
    
    return 0;
}

static net_ebb_request_t net_ebb_connection_new_request(void *data) {
    net_ebb_connection_t connection = data;
    if(connection->new_request)
        return connection->new_request(connection);
    return NULL;
}

static void net_ebb_connection_timeout(net_timer_t timer, void * ctx) {
    net_ebb_connection_t connection = ctx;

    //printf("on_timeout\n");

    /* if on_timeout returns true, we don't time out */
    if (connection->on_timeout) {
        int r = connection->on_timeout(connection);

        if (r == EBB_AGAIN) {
            net_ebb_connection_reset_timeout(connection);
            return;
        }
    }

    net_endpoint_close_after_send(connection->m_endpoint);
}

void net_ebb_connection_reset_timeout(net_ebb_connection_t connection) {
    //TODO:
    //net_timer_active(connection->server->loop, &connection->timeout_watcher);
}
