#include "net_timer.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_nghttp2_session_i.h"
#include "net_nghttp2_request_i.h"

static net_nghttp2_request_t net_nghttp2_session_new_request(void *data);

int net_nghttp2_session_init(net_endpoint_t base_endpoint) {
    net_nghttp2_service_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_nghttp2_session_t session = net_endpoint_data(base_endpoint);

    /* net_nghttp2_request_parser_init(&session->parser); */
    /* session->parser.data = session; */
    /* session->parser.new_request = net_nghttp2_session_new_request; */

    TAILQ_INIT(&session->m_requests);
    
    net_nghttp2_session_timeout_reset(session);

    return 0;
}

void net_nghttp2_session_fini(net_endpoint_t base_endpoint) {
    //net_nghttp2_service_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_nghttp2_session_t session = net_endpoint_data(base_endpoint);

    while(!TAILQ_EMPTY(&session->m_requests)) {
        net_nghttp2_request_free(TAILQ_FIRST(&session->m_requests));
    }
}

int net_nghttp2_session_input(net_endpoint_t base_endpoint) {
    net_nghttp2_service_t service = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_nghttp2_session_t session = net_endpoint_data(base_endpoint);

    net_nghttp2_session_timeout_reset(session);

    while(net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        uint32_t data_size = 0;
        void* data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_read, &data_size);
        if (data == NULL || data_size == 0) return 0;

        //net_nghttp2_request_parser_execute(&session->parser, data, data_size);

        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return -1;
        
        net_endpoint_buf_consume(base_endpoint, net_ep_buf_read, data_size);
        
        /* parse error? just drop the client. screw the 400 response */
        /* if (net_nghttp2_request_parser_has_error(&session->parser)) { */
        /*     CPE_ERROR( */
        /*         service->m_em, "nghttp2: %s: request parser has error, auto close!", */
        /*         net_endpoint_dump(net_nghttp2_service_tmp_buffer(service), base_endpoint)); */
        /*     return -1; */
        /* } */
    }

    return -1;
}

net_nghttp2_service_t net_nghttp2_session_service(net_nghttp2_session_t session) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(session);
    return net_protocol_data(net_endpoint_protocol(base_endpoint));
}

static net_nghttp2_request_t net_nghttp2_session_new_request(void *data) {
    net_nghttp2_session_t session = data;
    return net_nghttp2_request_create(session);;
}

void net_nghttp2_session_check_remove_done_requests(net_nghttp2_session_t session) {
    net_nghttp2_request_t request = TAILQ_FIRST(&session->m_requests);

    while(request && request->m_state == net_nghttp2_request_state_complete) {
        net_nghttp2_request_free(request);

        request = TAILQ_FIRST(&session->m_requests);
        if (request) {
        }
    }
}
