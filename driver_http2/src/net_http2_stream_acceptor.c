#include <assert.h>
#include "net_schedule.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_http2_stream_acceptor_i.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_stream_using_i.h"
#include "net_http2_endpoint.h"
#include "net_http2_req.h"

int net_http2_stream_acceptor_on_new_stream(void * ctx, net_http2_req_t req);

int net_http2_stream_acceptor_on_new_endpoint(void * ctx, net_endpoint_t base_http_endpoint) {
    net_acceptor_t base_acceptor = ctx;
    net_driver_t base_driver = net_acceptor_driver(base_acceptor);
    net_http2_stream_driver_t driver = net_driver_data(base_driver);
    net_http2_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    net_http2_endpoint_t http_endpoint = net_http2_endpoint_cast(base_http_endpoint);
    assert(http_endpoint);

    if (net_http2_endpoint_set_runing_mode(http_endpoint, net_http2_endpoint_runing_mode_svr)) {
        CPE_ERROR(
            driver->m_em, "http2: %s: set control runing-mode fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_http_endpoint));
        return -1;
    }

    net_http2_endpoint_set_acceptor(
        http_endpoint,
        base_acceptor,
        net_http2_stream_acceptor_on_new_stream,
        NULL); /*释放在using的监听中 */
    
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

    TAILQ_INIT(&acceptor->m_usings);

    return 0;
}

void net_http2_stream_acceptor_fini(net_acceptor_t base_acceptor) {
    net_http2_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_http2_stream_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    if (acceptor->m_control_acceptor) {
        net_acceptor_free(acceptor->m_control_acceptor);
        acceptor->m_control_acceptor = NULL;
    }
}

int net_http2_stream_acceptor_on_new_stream(void * ctx, net_http2_req_t req) {
    net_acceptor_t base_acceptor = ctx;
    net_http2_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_http2_stream_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    net_http2_endpoint_t http2_endpoint = net_http2_req_endpoint(req);
    net_endpoint_t base_http2_endpoint = net_http2_endpoint_base_endpoint(http2_endpoint);

    if (net_http2_req_add_res_head(req, ":status", "200") != 0
        || net_http2_req_start_response(req, 1) != 0)
    {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: accept: send respoinse fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_http2_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode(http2_endpoint)));
        return -1;
    }
    
    net_endpoint_t base_stream =
        net_endpoint_create(
            net_driver_from_data(driver),
            net_acceptor_protocol(base_acceptor),
            NULL);
    if (base_stream == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: accept: create stream endpoint fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_http2_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode(http2_endpoint)));
        return -1;
    }

    net_http2_stream_endpoint_t stream = net_http2_stream_endpoint_cast(base_stream);

    net_http2_stream_endpoint_set_req(stream, req);
    
    if (net_http2_stream_endpoint_sync_state(stream) != 0) {
        net_http2_stream_endpoint_set_req(stream, NULL);
        net_endpoint_free(base_stream);
        return -1;
    }

    if (net_acceptor_on_new_endpoint(base_acceptor, stream->m_base_endpoint) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: established: on new endpoint fail",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode_svr));
        return -1;
    }

    return 0;
}
