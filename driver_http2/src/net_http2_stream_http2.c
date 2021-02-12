#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_http2_stream_i.h"
#include "net_http2_utils.h"

int net_http2_stream_commit_headers(net_http2_stream_t stream) {
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));
    assert(endpoint);

    assert(stream->m_stream_id == -1);
    
    net_address_t target_address = net_endpoint_remote_address(endpoint->m_base_endpoint);
    if (target_address == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: send connect request: no remote target!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        return -1;
    }

    char target_path[256];
    cpe_str_dup(
        target_path, sizeof(target_path),
        net_address_dump(net_http2_protocol_tmp_buffer(protocol), target_address));
    
    nghttp2_nv hdrs[] = {
        MAKE_NV2(":method", "CONNECT"),
        MAKE_NV(":authority", target_path, strlen(target_path)),
    };

    const nghttp2_priority_spec * pri_spec = NULL;

    int rv = nghttp2_submit_headers(
        endpoint->m_http2_session, NGHTTP2_FLAG_NONE, -1, pri_spec, hdrs, CPE_ARRAY_SIZE(hdrs), stream);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> submit request fail, %s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id,
            nghttp2_strerror(rv));
        return -1;
    }
    stream->m_stream_id = (int32_t)rv;

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> submit headers",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id);
    }

    //net_http2_endpoint_schedule_flush(control);

    return 0;
}
