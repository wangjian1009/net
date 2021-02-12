#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_http2_stream_i.h"

void net_http2_stream_free(net_http2_stream_t stream) {
}

void net_http2_stream_on_input(net_http2_stream_t stream, const uint8_t * data, uint32_t len) {
    /* if (net_endpoint_buf_append(stream->m_base_endpoint, net_ep_buf_read, data, (uint32_t)len) != 0) { */
    /*     if (net_endpoint_error_source(stream->m_base_endpoint) == net_endpoint_error_source_none) { */
    /*         net_endpoint_set_error( */
    /*             stream->m_base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic, NULL); */
    /*     } */
    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_error) != 0) { */
    /*         net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting); */
    /*     } */
    /* } */
}

void net_http2_stream_on_close(net_http2_stream_t stream, int http2_error) {
    /* if (stream->m_stream_id != -1) { */
    /*     assert(stream->m_stream_id == stream_id); */
    /*     if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) { */
    /*         CPE_INFO( */
    /*             protocol->m_em, "http2: %s: %s: http2: %d: stream close and ignore rst!", */
    /*             net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
    /*             net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), */
    /*             stream->m_stream_id); */
    /*     } */
    /*     stream->m_stream_id = -1; */
    /* } */
    
    /* if (error_code == 0) { */
    /*     if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) { */
    /*         CPE_INFO( */
    /*             protocol->m_em, "http2: %s: %s: http2: %d: stream closed no error", */
    /*             net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
    /*             net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), */
    /*             stream_id); */
    /*     } */

    /*     if (net_endpoint_error_source(stream->m_base_endpoint) == net_endpoint_error_source_none) { */
    /*         net_endpoint_set_error( */
    /*             stream->m_base_endpoint, */
    /*             net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed, NULL); */
    /*     } */

    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) { */
    /*         net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting); */
    /*     } */
    /* } */
    /* else { */
    /*     CPE_ERROR( */
    /*         protocol->m_em, "http2: %s: %s: http2: %d: stream closed, error=%s", */
    /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
    /*         net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), */
    /*         stream_id, net_http2_error_code_str(error_code)); */

    /*     if (net_endpoint_error_source(stream->m_base_endpoint) == net_endpoint_error_source_none) { */
    /*         net_endpoint_set_error( */
    /*             stream->m_base_endpoint, net_endpoint_error_source_network, */
    /*             net_endpoint_network_errno_remote_closed, net_http2_error_code_str(error_code)); */
    /*     } */

    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_error) != 0) { */
    /*         net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting); */
    /*     } */
    /* } */
}

int net_http2_stream_on_request_head(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len)
{
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (cpe_str_cmp_part((const char *)name, name_len, ":method") == 0) {
    }

    return 0;
}

int net_http2_stream_on_response_head(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len)
{
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (cpe_str_cmp_part((const char *)name, name_len, ":status") == 0) {
        char value_buf[64];
        if (value_len + 1 > CPE_ARRAY_SIZE(value_buf)) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: http2: %d: on head: status overflow!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                stream->m_stream_id);
            return -1;
        }

        const char * str_status = cpe_str_dup_len(value_buf, sizeof(value_buf), value, value_len);
        int status = atoi(str_status);
        if (status == 200) {
            return 0;
        }
        else {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: http2: %d: on head: direct fail, status=%.*s!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                stream->m_stream_id,
                (int)value_len, value);
            return -1;
        }
    }
    
    return 0;
}

int net_http2_stream_on_tailer(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len)
{
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (cpe_str_cmp_part((const char *)name, name_len, "reason") == 0) {
        //assert(stream->m_conn);

        /* if (sfox_client_conn_close_reason(stream->m_conn) == sfox_client_conn_close_reason_none) { */
        /*     char buf[128]; */
            
        /*     sfox_client_conn_set_close_reason( */
        /*         stream->m_conn, */
        /*         sfox_client_conn_close_reason_remote_close, */
        /*         cpe_str_dup_len(buf, sizeof(buf), value, value_len), NULL); */
        /* } */
    }

    return 0;
}
