#include "net_smux_stream_i.h"
#include "net_smux_session_i.h"

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

int net_smux_stream_eq(net_smux_stream_t l, net_smux_stream_t r, void * user_data) {
    return l->m_stream_id == r->m_stream_id ? 1 : 0;
}

uint32_t net_smux_stream_hash(net_smux_stream_t o, void * user_data) {
    return o->m_stream_id;
}
