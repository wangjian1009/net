#include "net_timer.h"
#include "net_dgram.h"
#include "net_endpoint.h"
#include "net_smux_session_i.h"
#include "net_smux_stream_i.h"
#include "net_smux_frame_i.h"

void net_smux_session_do_pint(net_timer_t timer, void * ctx);
void net_smux_session_do_timeout(net_timer_t timer, void * ctx);
void net_smux_session_dgram_input(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_smux_session_t
net_smux_session_create_i(
    net_smux_protocol_t protocol,
    net_smux_session_runing_mode_t runing_mode,
    net_smux_session_underline_type_t underline_type)
{
    net_smux_session_t session = mem_alloc(protocol->m_alloc, sizeof(struct net_smux_session));
    if (session == NULL) {
        CPE_ERROR(protocol->m_em, "smux: session: create: alloc failed");
        return NULL;
    }

    session->m_protocol = protocol;
    session->m_session_id = ++protocol->m_max_session_id;
    session->m_runing_mode = runing_mode;
    session->m_max_stream_id = runing_mode == net_smux_session_runing_mode_cli ? 1 : 0;
    session->m_timer_ping = NULL;
    session->m_timer_timeout = NULL;

    session->m_underline.m_type = underline_type;
    switch(session->m_underline.m_type) {
    case net_smux_session_underline_udp:
        session->m_underline.m_udp.m_dgram = NULL;
        break;
    case net_smux_session_underline_tcp:
        session->m_underline.m_tcp.m_endpoint = NULL;
        break;
    }

    if (cpe_hash_table_init(
            &session->m_streams,
            protocol->m_alloc,
            (cpe_hash_fun_t) net_smux_stream_hash,
            (cpe_hash_eq_t) net_smux_stream_eq,
            CPE_HASH_OBJ2ENTRY(net_smux_stream, m_hh_for_session),
            -1) != 0)
    {
        CPE_ERROR(protocol->m_em, "smux: session %d: create: init hash table fail!", session->m_session_id);
        mem_free(protocol->m_alloc, session);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&protocol->m_sessions, session, m_next);

    /**/
    if (!protocol->m_cfg_keep_alive_disabled) {
        session->m_timer_ping = net_timer_auto_create(
            net_smux_protocol_schedule(protocol), net_smux_session_do_pint, session);
        if (session->m_timer_ping == NULL) {
            CPE_ERROR(protocol->m_em, "smux: session %d: create: create ping timer fail!", session->m_session_id);
            net_smux_session_free(session);
            return NULL;
        }

        session->m_timer_timeout = net_timer_auto_create(
            net_smux_protocol_schedule(protocol), net_smux_session_do_pint, session);
        if (session->m_timer_timeout == NULL) {
            CPE_ERROR(protocol->m_em, "smux: session %d: create: create timeout timer fail!", session->m_session_id);
            net_smux_session_free(session);
            return NULL;
        }
    }

    return session;
}

net_smux_session_t
net_smux_session_create_udp(
    net_smux_protocol_t protocol, net_smux_session_runing_mode_t runing_mode,
    net_driver_t driver, net_address_t address)
{
    net_smux_session_t session = net_smux_session_create_i(protocol, runing_mode, net_smux_session_underline_udp);
    if (session == NULL) return NULL;

    session->m_underline.m_udp.m_dgram =
        net_dgram_create(driver, address, net_smux_session_dgram_input, session);
    if (session->m_underline.m_udp.m_dgram == NULL) {
        CPE_ERROR(protocol->m_em, "smux: session %d: create: create dgram fail!", session->m_session_id);
        net_smux_session_free(session);
        return NULL;
    }

    return session;
}

void net_smux_session_free(net_smux_session_t session) {
    net_smux_protocol_t protocol = session->m_protocol;

    switch(session->m_underline.m_type) {
    case net_smux_session_underline_udp:
        if (session->m_underline.m_udp.m_dgram) {
            net_dgram_free(session->m_underline.m_udp.m_dgram);
            session->m_underline.m_udp.m_dgram = NULL;
        }
        break;
    case net_smux_session_underline_tcp:
        if (session->m_underline.m_tcp.m_endpoint) {
            net_endpoint_free(session->m_underline.m_tcp.m_endpoint);
            session->m_underline.m_tcp.m_endpoint = NULL;
        }
        break;
    }

    if (session->m_timer_ping) {
        net_timer_free(session->m_timer_ping);
        session->m_timer_ping = NULL;
    }
    
    if (session->m_timer_timeout) {
        net_timer_free(session->m_timer_timeout);
        session->m_timer_timeout = NULL;
    }
    
    net_smux_stream_free_all(session);
    cpe_hash_table_fini(&session->m_streams);
    
    TAILQ_REMOVE(&protocol->m_sessions, session, m_next);
    mem_free(protocol->m_alloc, session);    
}

net_smux_session_runing_mode_t net_smux_session_runing_mode(net_smux_session_t session) {
    return session->m_runing_mode;
}

net_smux_session_underline_type_t net_smux_session_underline_type(net_smux_session_t session) {
    return session->m_underline.m_type;
}

net_address_t net_smux_session_address(net_smux_session_t session) {
    switch(session->m_underline.m_type) {
    case net_smux_session_underline_udp:
        return session->m_underline.m_udp.m_dgram
            ? net_dgram_address(session->m_underline.m_udp.m_dgram)
            : NULL;
    case net_smux_session_underline_tcp:
        return session->m_underline.m_tcp.m_endpoint
            ? net_endpoint_address(session->m_underline.m_tcp.m_endpoint)
            : NULL;
        break;
    }
}

void net_smux_session_do_pint(net_timer_t timer, void * ctx) {
}

void net_smux_session_do_timeout(net_timer_t timer, void * ctx) {
}

void net_smux_session_dgram_input(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source) {
}

int net_smux_session_write_frame(net_smux_session_t session, net_smux_cmd_t cmd, uint32_t sid) {
    return 0;
}

const char * net_smux_session_runing_mode_str(net_smux_session_runing_mode_t runing_mode) {
    switch (runing_mode) {
    case net_smux_session_runing_mode_cli:
        return "cli";
    case net_smux_session_runing_mode_svr:
        return "svr";
    }
}
