#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_driver.h"
#include "net_address.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_endpoint_i.h"
#include "net_http2_utils.h"

int net_http2_stream_endpoint_send_connect_request(net_http2_stream_endpoint_t endpoint) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(endpoint->m_base_endpoint));
    net_http2_endpoint_t control = endpoint->m_control;
    assert(control);

    assert(endpoint->m_stream_id != -1);
    
    net_address_t target_address = net_endpoint_remote_address(endpoint->m_base_endpoint);
    if (target_address == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: send connect request: no remote target!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), endpoint->m_base_endpoint));
        return -1;
    }

    char target_path[256];
    cpe_str_dup(
        target_path, sizeof(target_path),
        net_address_dump(net_http2_stream_driver_tmp_buffer(driver), target_address));
    
    nghttp2_nv hdrs[] = {
        MAKE_NV2(":method", "CONNECT"),
        MAKE_NV(":authority", target_path, strlen(target_path)),
    };

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            driver->m_em, "http2: %s: request: ==> begin connect",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), endpoint->m_base_endpoint));
    }

    const nghttp2_priority_spec * pri_spec = NULL;

    int rv = nghttp2_submit_headers(
        control->m_http2_session, NGHTTP2_FLAG_NONE, -1, pri_spec, hdrs, CPE_ARRAY_SIZE(hdrs), endpoint);
    if (rv < 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: request: submit request fail, %s!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), endpoint->m_base_endpoint),
            nghttp2_strerror(rv));
        return -1;
    }
    endpoint->m_stream_id = (int32_t)rv;

    /* sfox_sfox_cli_router_schedule_flush(sfox_router); */

    return 0;
}

