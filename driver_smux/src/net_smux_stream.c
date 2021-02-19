#include "net_smux_stream_i.h"
#include "net_smux_session_i.h"
#include "net_smux_pro.h"

net_smux_stream_t
net_smux_stream_create(net_smux_session_t session, uint32_t stream_id) {
    net_smux_protocol_t protocol = session->m_protocol;

    net_smux_stream_t stream = mem_alloc(protocol->m_alloc, sizeof(struct net_smux_stream));
    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "smux: session %d: stream %d: create: alloc fail",
            session->m_session_id, session->m_max_stream_id);
        return NULL;
    }

    stream->m_session = session;
    stream->m_stream_id = stream_id;
    stream->m_state = net_smux_stream_state_init;

	stream->m_num_read = 0;
	stream->m_num_written = 0;
	stream->m_incr = 0;

	stream->m_peer_consumed = 0;
	stream->m_peer_window = 0;

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

    if (stream->m_read_ctx_free) {
        stream->m_read_ctx_free(stream->m_read_ctx);
        stream->m_read_ctx_free = NULL;
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

net_smux_stream_state_t net_smux_stream_state(net_smux_stream_t stream) {
    return stream->m_state;
}

void net_smux_stream_close_and_free(net_smux_stream_t stream) {
    net_smux_session_send_frame(
        stream->m_session, stream,
        net_smux_cmd_fin, NULL, 0,
        0, 0);
    net_smux_stream_free(stream);
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

void net_smux_stream_update_pear(net_smux_stream_t stream, uint32_t consumed, uint32_t window) {
    stream->m_peer_consumed = consumed;
    stream->m_peer_window = window;
}

int net_smux_stream_eq(net_smux_stream_t l, net_smux_stream_t r, void * user_data) {
    return l->m_stream_id == r->m_stream_id ? 1 : 0;
}

uint32_t net_smux_stream_hash(net_smux_stream_t o, void * user_data) {
    return o->m_stream_id;
}

const char * net_smux_stream_state_str(net_smux_stream_state_t state) {
    switch (state) {
    case net_smux_stream_state_init:
        return "init";
    case net_smux_stream_state_closed:
        return "closed";
    }
}
