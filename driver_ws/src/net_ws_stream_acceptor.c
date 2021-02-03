#include <assert.h>
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_ws_stream_acceptor_i.h"
#include "net_ws_stream_endpoint_i.h"
#include "net_ws_endpoint_i.h"

int net_ws_stream_acceptor_on_new_endpoint(void * ctx, net_endpoint_t base_endpoint) {
    net_acceptor_t base_acceptor = ctx;
    net_driver_t base_driver = net_acceptor_driver(base_acceptor);
    net_ws_stream_driver_t driver = net_driver_data(base_driver);
    net_ws_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    CPE_ERROR(driver->m_em, "net: ws: xxxxx: on new endpoint");

    net_ws_endpoint_t underline = net_ws_endpoint_cast(base_endpoint);
    assert(underline);

    net_endpoint_t stream_base_endpoint = NULL;
    net_ws_stream_endpoint_t stream = NULL;

    stream_base_endpoint =
        net_endpoint_create(base_driver, net_acceptor_protocol(base_acceptor), NULL);
    if (stream_base_endpoint == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: create stream endpoint fail",
            net_endpoint_dump(net_ws_driver_tmp_buffer(driver), base_endpoint));
        goto INIT_ERROR;
    }
    stream = net_ws_stream_endpoint_cast(stream_base_endpoint);
    assert(stream);

    if (net_ws_endpoint_set_runing_mode(underline, net_ws_endpoint_runing_mode_svr)) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: set underline runing-mode fail",
            net_endpoint_dump(net_ws_driver_tmp_buffer(driver), base_endpoint));
        goto INIT_ERROR;
    }

    stream->m_underline = underline;
    underline->m_stream = stream;
    
    if (net_endpoint_set_state(stream_base_endpoint, net_endpoint_state_established) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: set new endpoint established fail",
            net_endpoint_dump(net_ws_driver_tmp_buffer(driver), base_endpoint));
        goto INIT_ERROR;
    }

    if (net_acceptor_on_new_endpoint(base_acceptor, stream_base_endpoint) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: on acceptor new endpoint fail",
            net_endpoint_dump(net_ws_driver_tmp_buffer(driver), base_endpoint));
        goto INIT_ERROR;
    }
    
    return 0;

INIT_ERROR:
    if (stream_base_endpoint) {
        stream->m_underline = NULL;
        net_endpoint_free(stream_base_endpoint);
    }
    underline->m_stream = NULL;
    return -1;
}

int net_ws_stream_acceptor_init(net_acceptor_t base_acceptor) {
    net_ws_stream_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_ws_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    
    acceptor->m_base_acceptor = 
        net_acceptor_create(
            driver->m_underline_driver,
            driver->m_underline_protocol,
            net_acceptor_address(base_acceptor),
            net_acceptor_queue_size(base_acceptor),
            net_ws_stream_acceptor_on_new_endpoint, base_acceptor);
    if (base_acceptor == NULL) {
        CPE_ERROR(driver->m_em, "net: ws: init: create inner acceptor fail");
        return -1;
    }

    return 0;
}

void net_ws_stream_acceptor_fini(net_acceptor_t base_acceptor) {
    net_ws_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_ws_stream_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    if (acceptor->m_base_acceptor) {
        net_acceptor_free(acceptor->m_base_acceptor);
        acceptor->m_base_acceptor = NULL;
    }
}
