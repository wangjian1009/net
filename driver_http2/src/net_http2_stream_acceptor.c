#include <assert.h>
#include "net_schedule.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_http2_stream_acceptor_i.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_endpoint_i.h"

int net_http2_stream_acceptor_on_new_endpoint(void * ctx, net_endpoint_t base_control) {
    net_acceptor_t base_acceptor = ctx;
    net_driver_t base_driver = net_acceptor_driver(base_acceptor);
    net_http2_stream_driver_t driver = net_driver_data(base_driver);
    net_http2_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    net_http2_endpoint_t control = net_http2_endpoint_cast(base_control);
    assert(control);

    net_endpoint_t base_endpoint = NULL;

    if (net_http2_endpoint_set_runing_mode(control, net_http2_endpoint_runing_mode_svr)) {
        CPE_ERROR(
            driver->m_em, "http2: %s: set control runing-mode fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_control));
        return -1;
    }

    //net_http2_endpoint_set_stream_acceptor(control, acceptor);
    
    return 0;
}

int net_http2_stream_acceptor_init(net_acceptor_t base_acceptor) {
    net_http2_stream_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_http2_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    net_address_t address = net_acceptor_address(base_acceptor);
    
    acceptor->m_control_acceptor = 
        net_acceptor_create(
            driver->m_control_driver,
            driver->m_control_protocol,
            address,
            net_acceptor_queue_size(base_acceptor),
            net_http2_stream_acceptor_on_new_endpoint, base_acceptor);
    if (base_acceptor == NULL) {
        CPE_ERROR(driver->m_em, "http2: init: create inner acceptor fail");
        return -1;
    }

    if (net_address_port(address) == 0) {
        net_address_set_port(address, net_address_port(net_acceptor_address(acceptor->m_control_acceptor)));
    }

    TAILQ_INIT(&acceptor->m_endpoints);

    return 0;
}

void net_http2_stream_acceptor_fini(net_acceptor_t base_acceptor) {
    net_http2_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_http2_stream_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    if (acceptor->m_control_acceptor) {
        net_acceptor_free(acceptor->m_control_acceptor);
        acceptor->m_control_acceptor = NULL;
    }

    while(!TAILQ_EMPTY(&acceptor->m_endpoints)) {
        net_http2_endpoint_t endpoint = TAILQ_FIRST(&acceptor->m_endpoints);
        //net_http2_endpoint_set_stream_acceptor(endpoint, NULL);
    }
}

net_http2_stream_endpoint_t
net_http2_stream_acceptor_accept(
    net_http2_stream_acceptor_t acceptor, net_http2_endpoint_t control, int32_t stream_id)
{
    net_acceptor_t base_acceptor = net_acceptor_from_data(acceptor);
    net_http2_stream_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    net_endpoint_t base_stream =
        net_endpoint_create(
            net_driver_from_data(driver),
            net_acceptor_protocol(base_acceptor),
            NULL);
    if (base_stream == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: accept: create stream endpoint fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), control->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode_svr));
        return NULL;
    }

    if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: accept: set connecting fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), control->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode_svr));
        net_endpoint_free(base_stream);
        return NULL;
    }
    
    net_http2_stream_endpoint_t stream = net_http2_stream_endpoint_cast(base_stream);
    net_http2_stream_endpoint_set_control(stream, control);
    stream->m_stream_id = stream_id;

    return stream;
}

int net_http2_stream_acceptor_established(
    net_http2_stream_acceptor_t acceptor, net_http2_stream_endpoint_t stream)
{
    net_acceptor_t base_acceptor = net_acceptor_from_data(acceptor);
    net_http2_stream_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    
    if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_established) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: %d: established: set endpoint established fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode_svr),
            stream->m_stream_id);
        return -1;
    }

    if (net_acceptor_on_new_endpoint(base_acceptor, stream->m_base_endpoint) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: %d: established: on new endpoint fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode_svr),
            stream->m_stream_id);
        return -1;
    }

    //if (net_http2_stream_endpoint_send_connect_response(stream) != 0) return -1;
    
    return 0;
}

            /* if (endpoint->m_svr.m_stream_acceptor == NULL) { */
            /*     CPE_ERROR( */
            /*         protocol->m_em, "http2: %s: %s: http2: %d: <== receive request header no acceptor", */
            /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
            /*         net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), */
            /*         frame->hd.stream_id); */
            /*     return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE; */
            /* } */

            /* stream = net_http2_stream_acceptor_accept(endpoint->m_svr.m_stream_acceptor, endpoint, frame->hd.stream_id); */
            /* if (stream == NULL) { */
            /*     CPE_ERROR( */
            /*         protocol->m_em, "http2: %s: %s: http2: %d: <== receive request header create new stream fail", */
            /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
            /*         net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), */
            /*         frame->hd.stream_id); */
            /*     return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE; */
            /* } */

            /* int rv = nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, stream); */
            /* if (rv != NGHTTP2_NO_ERROR) { */
            /*     CPE_ERROR( */
            /*         protocol->m_em, "http2: %s: %s: http2: %d: <== receive request header bind stream fail, error=%s", */
            /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
            /*         net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), */
            /*         frame->hd.stream_id, net_http2_error_code_str(rv)); */
            /*     return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE; */
            /* } */
