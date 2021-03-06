#include "cpe/utils/stream_buffer.h"
#include "net_smux_stream_i.h"
#include "net_smux_session_i.h"
#include "net_smux_pro.h"

net_smux_stream_t
net_smux_stream_create(net_smux_session_t session, uint32_t stream_id) {
    net_smux_protocol_t protocol = session->m_protocol;

    net_smux_stream_t stream = mem_alloc(protocol->m_alloc, sizeof(struct net_smux_stream));
    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "smux: %s: %d: create: alloc fail",
            net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session),
            stream_id);
        return NULL;
    }

    stream->m_session = session;
    stream->m_stream_id = stream_id;
    stream->m_is_processing = 0;
    stream->m_is_free = 0;

    SIMPLEQ_INIT(&stream->m_read_blocks);

	stream->m_num_read = 0;
	stream->m_num_written = 0;
	stream->m_incr = 0;

	stream->m_peer_consumed = 0;
	stream->m_peer_window = 0;

    stream->m_write_ctx = NULL;
    stream->m_on_write_resume = NULL;
    stream->m_write_ctx_free = NULL;
    
    stream->m_read_ctx = NULL;
    stream->m_on_recv = NULL;
    stream->m_read_ctx_free = NULL;
    
    cpe_hash_entry_init(&stream->m_hh_for_session);
    if (cpe_hash_table_insert_unique(&session->m_streams, stream) != 0) {
        CPE_ERROR(
            protocol->m_em, "smux: session %d: stream %d: create: insert fail",
            session->m_session_id, session->m_max_stream_id);
        mem_free(protocol->m_alloc, stream);
        return NULL;
    }

    return stream;
}

void net_smux_stream_free(net_smux_stream_t stream) {
    net_smux_session_t session = stream->m_session;
    net_smux_protocol_t protocol = session->m_protocol;

    if (stream->m_is_processing) {
        stream->m_is_free = 1;
        return;
    }

    int32_t return_tokens = 0;
    while(!SIMPLEQ_EMPTY(&stream->m_read_blocks)) {
        net_smux_mem_block_t block = SIMPLEQ_FIRST(&stream->m_read_blocks);
        return_tokens += block->m_size;
        SIMPLEQ_REMOVE_HEAD(&stream->m_read_blocks, m_next);
        net_smux_mem_cache_release(session->m_mem_cache, block);
    }
    net_smux_session_return_tokens(session, return_tokens);

    if (stream->m_read_ctx_free) {
        stream->m_read_ctx_free(stream->m_read_ctx);
        stream->m_read_ctx_free = NULL;
    }

    if (stream->m_write_ctx_free) {
        stream->m_write_ctx_free(stream->m_write_ctx);
        stream->m_write_ctx_free = NULL;
    }
    
    cpe_hash_table_remove_by_ins(&session->m_streams, stream);

    mem_free(protocol->m_alloc, stream);
}

void net_smux_stream_free_all(net_smux_session_t session) {
    struct cpe_hash_it stream_it;
    net_smux_stream_t stream;

    cpe_hash_it_init(&stream_it, &session->m_streams);

    stream = cpe_hash_it_next(&stream_it);
    while(stream) {
        net_smux_stream_t next = cpe_hash_it_next(&stream_it);
        net_smux_stream_free(stream);
        stream = next;
    }
}

uint32_t net_smux_stream_id(net_smux_stream_t stream) {
    return stream->m_stream_id;
}

net_smux_session_t net_smux_stream_session(net_smux_stream_t stream) {
    return stream->m_session;
}

void net_smux_stream_close_and_free(net_smux_stream_t stream) {
    net_smux_session_send_frame(stream->m_session, stream, net_smux_cmd_fin, NULL, 0, 0, 0);
    net_smux_stream_free(stream);
}

void net_smux_stream_update_pear(net_smux_stream_t stream, uint32_t consumed, uint32_t window) {
    stream->m_peer_consumed = consumed;
    stream->m_peer_window = window;
}

void net_smux_stream_print(write_stream_t ws, net_smux_stream_t stream) {
    net_smux_session_print(ws, stream->m_session);
    stream_printf(ws, ": %d", stream->m_stream_id);
}

const char * net_smux_stream_dump(mem_buffer_t buffer, net_smux_stream_t stream) {
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    net_smux_stream_print((write_stream_t)&ws, stream);
    stream_putc((write_stream_t)&ws, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

int net_smux_stream_eq(net_smux_stream_t l, net_smux_stream_t r, void * user_data) {
    return l->m_stream_id == r->m_stream_id ? 1 : 0;
}

uint32_t net_smux_stream_hash(net_smux_stream_t o, void * user_data) {
    return o->m_stream_id;
}
