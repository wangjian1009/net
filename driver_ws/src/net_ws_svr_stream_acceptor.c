#include <assert.h>
#include "cpe/utils/random.h"
#include "cpe/utils/hex_utils.h"
#include "net_schedule.h"
#include "net_acceptor.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ws_svr_stream_acceptor_i.h"
#include "net_ws_svr_stream_endpoint_i.h"
#include "net_ws_svr_endpoint_i.h"

int net_ws_svr_stream_acceptor_on_new_endpoint(void * ctx, net_endpoint_t base_underline) {
    net_acceptor_t base_acceptor = ctx;
    net_driver_t base_driver = net_acceptor_driver(base_acceptor);
    net_ws_svr_stream_driver_t driver = net_driver_data(base_driver);
    net_schedule_t schedule = net_endpoint_schedule(base_underline);

    net_ws_svr_endpoint_t underline = net_endpoint_protocol_data(base_underline);
    assert(underline->m_stream == NULL);

    net_endpoint_t new_base_endpoint =
        net_endpoint_create(
            net_acceptor_driver(base_acceptor),
            net_acceptor_protocol(base_acceptor),
            NULL);
    if (new_base_endpoint == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: on new endpoint: create endpoint error!",
            net_driver_name(base_driver));
        return -1;
    }

    net_ws_svr_stream_endpoint_t new_endpoint = net_endpoint_data(new_base_endpoint);
    new_endpoint->m_underline = base_underline;
    underline->m_stream = new_endpoint;

    if (net_endpoint_set_state(new_base_endpoint, net_endpoint_state_connecting) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: on new endpoint: set established failed!",
            net_driver_name(base_driver));
        new_endpoint->m_underline = NULL;
        underline->m_stream = NULL;
        net_endpoint_free(new_base_endpoint);
        return -1;
    }

    if (net_acceptor_on_new_endpoint(base_acceptor, new_base_endpoint) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: on new endpoint: set established failed!",
            net_driver_name(base_driver));
        new_endpoint->m_underline = NULL;
        underline->m_stream = NULL;
        net_endpoint_free(new_base_endpoint);
        return -1;
    }

    if (net_endpoint_driver_debug(new_base_endpoint) > net_endpoint_protocol_debug(base_underline)) {
        net_endpoint_set_protocol_debug(base_underline, net_endpoint_driver_debug(new_base_endpoint));
    }

    return 0;
}

int net_ws_svr_stream_acceptor_init(net_acceptor_t base_acceptor) {
    net_ws_svr_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_driver_t base_driver = net_acceptor_driver(base_acceptor);
    net_ws_svr_stream_driver_t driver = net_driver_data(base_driver);

    acceptor->m_underline =
        net_acceptor_create(
            driver->m_underline_driver,
            driver->m_underline_protocol,
            net_acceptor_address(base_acceptor),
            net_acceptor_queue_size(base_acceptor),
            net_ws_svr_stream_acceptor_on_new_endpoint,
            base_acceptor);
    if (acceptor->m_underline == NULL) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: stream acceptor: init: create base acceptor failed!",
            net_driver_name(base_driver));
        return -1;
    }

    return 0;
}

void net_ws_svr_stream_acceptor_fini(net_acceptor_t base_acceptor) {
    net_ws_svr_stream_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    if (acceptor->m_underline) {
        net_acceptor_free(acceptor->m_underline);
        acceptor->m_underline = NULL;
    }
}
