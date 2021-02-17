#include "net_smux_stream_i.h"
#include "net_smux_session_i.h"

net_smux_stream_t net_smux_stream_create(net_smux_session_t session) {
    net_smux_manager_t manager = session->m_manager;
        
    net_smux_stream_t stream
        = mem_alloc(manager->m_alloc, sizeof(struct net_smux_stream));
    if (stream == NULL) {
    }

    return stream;
}

void net_smux_stream_free(net_smux_stream_t stream) {
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

int net_smux_stream_eq(net_smux_stream_t l, net_smux_stream_t r, void * user_data) {
    return l->m_stream_id == r->m_stream_id ? 1 : 0;
}

uint32_t net_smux_stream_hash(net_smux_stream_t o, void * user_data) {
    return o->m_stream_id;
}
