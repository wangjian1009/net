#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_http2_endpoint_stream_i.h"

int net_http2_endpoint_stream_on_head(
    net_http2_endpoint_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len)
{
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(stream->m_endpoint->m_base_endpoint));

    if (cpe_str_cmp_part((const char *)name, name_len, ":status") == 0) {
        char value_buf[64];
        if (value_len + 1 > CPE_ARRAY_SIZE(value_buf)) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: on head: status overflow!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), stream->m_endpoint->m_base_endpoint));
            return -1;
        }

        const char * str_status = cpe_str_dup_len(value_buf, sizeof(value_buf), value, value_len);
        int status = atoi(str_status);
        if (status == 200) {
            return 0;
        }
        else {
            CPE_ERROR(
                protocol->m_em, "http2: %s: on head: direct fail, status=%.*s!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), stream->m_endpoint->m_base_endpoint),
                (int)value_len, value);
            return -1;
        }
    }
    
    return 0;
}

int net_http2_endpoint_stream_on_tailer(
    net_http2_endpoint_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len)
{
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(stream->m_endpoint->m_base_endpoint));

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

