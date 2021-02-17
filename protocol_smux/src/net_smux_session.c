#include "net_smux_session_i.h"
#include "net_smux_stream_i.h"

net_smux_session_t
net_smux_session_create(net_smux_manager_t manager, net_smux_session_runing_mode_t runing_mode) {
    net_smux_session_t session = mem_alloc(manager->m_alloc, sizeof(struct net_smux_session));
    if (session == NULL) {
        CPE_ERROR(manager->m_em, "smux: session: create: alloc failed");
        return NULL;
    }

    session->m_manager = manager;
    session->m_runing_mode = runing_mode;

    if (cpe_hash_table_init(
            &session->m_streams,
            manager->m_alloc,
            (cpe_hash_fun_t) net_smux_stream_hash,
            (cpe_hash_eq_t) net_smux_stream_eq,
            CPE_HASH_OBJ2ENTRY(net_smux_stream, m_hh_for_session),
            -1) != 0)
    {
        CPE_ERROR(manager->m_em, "smux: session: create: init hash table fail!");
        mem_free(manager->m_alloc, session);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&manager->m_sessions, session, m_next);
    return session;
}

void net_smux_session_free(net_smux_session_t session) {
    net_smux_manager_t manager = session->m_manager;

    net_smux_stream_free_all(session);
    cpe_hash_table_fini(&session->m_streams);
    
    TAILQ_REMOVE(&manager->m_sessions, session, m_next);
    mem_free(manager->m_alloc, session);    
}

net_smux_session_runing_mode_t net_smux_session_runing_mode(net_smux_session_t session) {
    return session->m_runing_mode;
}

const char * net_smux_session_runing_mode_str(net_smux_session_runing_mode_t runing_mode) {
    switch (runing_mode) {
    case net_smux_session_runing_mode_cli:
        return "cli";
    case net_smux_session_runing_mode_svr:
        return "svr";
    }
}
