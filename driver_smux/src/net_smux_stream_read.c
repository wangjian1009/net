#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "net_smux_stream_i.h"
#include "net_smux_mem_cache_i.h"

void net_smux_stream_input(net_smux_stream_t stream, void const * data, uint16_t data_len) {
    net_smux_session_t session = stream->m_session;
    net_smux_protocol_t protocol = session->m_protocol;

    /*没有缓存，尝试直接发送 */    
    if (SIMPLEQ_EMPTY(&stream->m_read_blocks) && stream->m_on_recv) {
        uint8_t tag_local = 0;
        if (!stream->m_is_processing) {
            stream->m_is_processing = 1;
            tag_local = 1;

            int rv = stream->m_on_recv(stream->m_read_ctx, stream, data, data_len);

            if (tag_local) {
                stream->m_is_processing = 0;
                if (stream->m_is_free) {
                    net_smux_stream_free(stream);
                    return;
                }
            }

            if (rv <= 0) {
                if (rv != net_smux_stream_write_error_wouldblock) {
                    CPE_ERROR(
                        protocol->m_em, "smux: %s: input: no recv error, rv=%d!",
                        net_smux_stream_dump(net_smux_protocol_tmp_buffer(protocol), stream), rv);
                    return;
                }
                /*阻塞情况下， 走后续的缓存策略 */
            }
            else {
                assert(rv <= data_len);

                if (rv == data_len) {
                    /*所有数据消耗完毕，直接返回，无需缓存*/
                    return;
                }
                else {
                    /*剩余数据需要缓存 */
                    data = ((const uint8_t *)data) + rv;
                    data_len -= rv;
                }
            }
        }
    }

    /*开始缓存数据 */
    net_smux_mem_block_t block = net_smux_mem_cache_alloc(session->m_mem_cache, data_len);
    if (block == NULL) {
        CPE_ERROR(
            protocol->m_em, "smux: %s: input: alloc block error!",
            net_smux_stream_dump(net_smux_protocol_tmp_buffer(protocol), stream));
        return;
    }

    memcpy(block + 1, data, data_len);
    SIMPLEQ_INSERT_TAIL(&stream->m_read_blocks, block, m_next);

    net_smux_session_get_tokens(session, (int32_t)data_len);
}

void net_smux_stream_read_resume(net_smux_stream_t stream) {
    net_smux_session_t session = stream->m_session;
    net_smux_protocol_t protocol = session->m_protocol;
    
    uint8_t tag_local = 0;
    if (!stream->m_is_processing) {
        stream->m_is_processing = 1;
        tag_local = 1;
    }

    int32_t return_tokens = 0;
    
    while(!SIMPLEQ_EMPTY(&stream->m_read_blocks)) {
        net_smux_mem_block_t block = SIMPLEQ_FIRST(&stream->m_read_blocks);

        if (stream->m_on_recv) {
            CPE_ERROR(
                protocol->m_em, "smux: %s: read resumen: no receiver!",
                net_smux_stream_dump(net_smux_protocol_tmp_buffer(protocol), stream));
            break;
        }

        int rv = stream->m_on_recv(stream->m_read_ctx, stream, block + 1, block->m_size);
        if (stream->m_is_free) break;
        
        if (rv <= 0) {
            if (rv == net_smux_stream_write_error_wouldblock) {
                break;
            }
            else {
                CPE_ERROR(
                    protocol->m_em, "smux: %s: read resumen: no recv error, rv=%d!",
                    net_smux_stream_dump(net_smux_protocol_tmp_buffer(protocol), stream), rv);
                break;
            }
        }

        assert(rv <= block->m_size);
        block->m_size -= rv;
        return_tokens += rv;
        
        if (block->m_size > 0) break;

        SIMPLEQ_REMOVE_HEAD(&stream->m_read_blocks, m_next);
    }

    if (tag_local) {
        stream->m_is_processing = 0;
        if (stream->m_is_free) {
            net_smux_stream_free(stream);
        }
    }

    net_smux_session_return_tokens(session, return_tokens);
}

void net_smux_stream_set_reader(
    net_smux_stream_t stream,
    void * read_ctx,
    net_smux_stream_on_recv_fun_t on_recv,
    void (*read_ctx_free)(void *))
{
    if (stream->m_read_ctx_free) {
        stream->m_read_ctx_free(stream->m_read_ctx);
    }
    
    stream->m_read_ctx = read_ctx;
    stream->m_on_recv = on_recv;
    stream->m_read_ctx_free = read_ctx_free;
}

void net_smux_stream_clear_reader(net_smux_stream_t stream) {
    stream->m_read_ctx = NULL;
    stream->m_on_recv = NULL;
    stream->m_read_ctx_free = NULL;
}

void * net_smux_stream_reader_ctx(net_smux_stream_t stream) {
    return stream->m_read_ctx;
}
