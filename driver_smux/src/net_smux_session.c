#include <assert.h>
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/priority_queue.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_dgram.h"
#include "net_timer.h"
#include "net_endpoint.h"
#include "net_smux_session_i.h"
#include "net_smux_stream_i.h"
#include "net_smux_endpoint_i.h"
#include "net_smux_dgram_i.h"
#include "net_smux_frame_i.h"

struct net_smux_write_rquest {
    uint64_t m_prio;
    net_smux_frame_t m_frame;
    int64_t m_expire_time_ms;
};
int net_smux_write_request_prio_cmp(void const * l, void const * r);

void net_smux_session_do_ping(net_timer_t timer, void * ctx);
void net_smux_session_do_timeout(net_timer_t timer, void * ctx);

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
    session->m_bucket = protocol->m_cfg_max_session_buffer;
    session->m_shaper = NULL;
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

    session->m_shaper =
        cpe_priority_queue_create(
            protocol->m_alloc,
            protocol->m_em,
            sizeof(struct net_smux_write_rquest),
            net_smux_write_request_prio_cmp,
            2048);
    if (session->m_shaper == NULL) {
        CPE_ERROR(protocol->m_em, "smux: session %d: create: create shaper fail!", session->m_session_id);
        mem_free(protocol->m_alloc, session);
        return NULL;
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
        cpe_priority_queue_free(session->m_shaper);
        mem_free(protocol->m_alloc, session);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&protocol->m_sessions, session, m_next);

    /**/
    if (!protocol->m_cfg_keep_alive_disabled) {
        session->m_timer_ping = net_timer_auto_create(
            net_smux_protocol_schedule(protocol), net_smux_session_do_ping, session);
        if (session->m_timer_ping == NULL) {
            CPE_ERROR(protocol->m_em, "smux: session %d: create: create ping timer fail!", session->m_session_id);
            net_smux_session_free(session);
            return NULL;
        }

        session->m_timer_timeout = net_timer_auto_create(
            net_smux_protocol_schedule(protocol), net_smux_session_do_ping, session);
        if (session->m_timer_timeout == NULL) {
            CPE_ERROR(protocol->m_em, "smux: session %d: create: create timeout timer fail!", session->m_session_id);
            net_smux_session_free(session);
            return NULL;
        }
    }

    return session;
}

net_smux_session_t
net_smux_session_create_dgram(
    net_smux_protocol_t protocol, net_smux_dgram_t dgram, net_address_t remote_address)
{
    net_schedule_t schedule = net_smux_protocol_schedule(protocol);
    
    net_smux_session_t session =
        net_smux_session_create_i(protocol, dgram->m_runing_mode, net_smux_session_underline_udp);
    if (session == NULL) return NULL;

    session->m_underline.m_udp.m_remote_address = net_address_copy(schedule, remote_address);
    if (session->m_underline.m_udp.m_remote_address == NULL) {
        CPE_ERROR(protocol->m_em, "smux: session %d: create: dup remote address fail!", session->m_session_id);
        net_smux_session_free(session);
        return NULL;
    }

    session->m_underline.m_udp.m_dgram = dgram;

    cpe_hash_entry_init(&session->m_underline.m_udp.m_hh_for_dgram);
    if (cpe_hash_table_insert_unique(&dgram->m_sessions, session) != 0) {
        CPE_ERROR(
            protocol->m_em, "smux: session %d: create: insert to dgram fail",
            session->m_session_id);

        net_address_free(session->m_underline.m_udp.m_remote_address);
        session->m_underline.m_udp.m_remote_address = NULL;
        session->m_underline.m_udp.m_dgram = NULL;
        net_smux_session_free(session);
        return NULL;
    }
    
    return session;
}

void net_smux_session_free(net_smux_session_t session) {
    net_smux_protocol_t protocol = session->m_protocol;

    if (session->m_shaper) {
        struct net_smux_write_rquest * request;
        while((request = cpe_priority_queue_top(session->m_shaper))) {
            net_smux_frame_free(session, request->m_frame);
            cpe_priority_queue_pop(session->m_shaper);
        }
        cpe_priority_queue_free(session->m_shaper);
    }
    
    switch(session->m_underline.m_type) {
    case net_smux_session_underline_udp:
        if (session->m_underline.m_udp.m_dgram) {
            /* net_dgram_free(session->m_underline.m_udp.m_dgram); */
            /* session->m_underline.m_udp.m_dgram = NULL; */
        }
        break;
    case net_smux_session_underline_tcp:
        if (session->m_underline.m_tcp.m_endpoint) {
            net_smux_session_set_session(session->m_underline.m_tcp.m_endpoint, NULL);
            assert(session->m_underline.m_tcp.m_endpoint == NULL);
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

net_address_t net_smux_session_local_address(net_smux_session_t session) {
    switch(session->m_underline.m_type) {
    case net_smux_session_underline_udp:
        return session->m_underline.m_udp.m_dgram
            ? net_dgram_address(session->m_underline.m_udp.m_dgram->m_dgram)
            : NULL;
    case net_smux_session_underline_tcp:
        return session->m_underline.m_tcp.m_endpoint
            ? net_endpoint_address(session->m_underline.m_tcp.m_endpoint->m_base_endpoint)
            : NULL;
        break;
    }
}

net_smux_stream_t
net_smux_session_open_stream(net_smux_session_t session) {
    net_smux_stream_t stream = net_smux_stream_create(session);

    return stream;
}

net_smux_stream_t
net_smux_session_find_stream(net_smux_session_t session, uint32_t stream_id) {
    struct net_smux_stream key;
    key.m_stream_id = stream_id;
    return cpe_hash_table_find(&session->m_streams, &key);
}

void net_smux_session_do_ping(net_timer_t timer, void * ctx) {
}

void net_smux_session_do_timeout(net_timer_t timer, void * ctx) {
}

int net_smux_session_enqueue_frame(net_smux_session_t session, net_smux_frame_t frame, uint64_t expire_ms, uint64_t prio) {
    net_smux_protocol_t protocol = session->m_protocol;

    struct net_smux_write_rquest write_request = { prio, frame, 0 };

    if (expire_ms) {
        net_schedule_t schedule = net_smux_protocol_schedule(protocol);
        write_request.m_expire_time_ms = net_schedule_cur_time_ms(schedule) + expire_ms;
    }
    
    if (cpe_priority_queue_insert(session->m_shaper, &write_request) != 0) {
        CPE_ERROR(protocol->m_em, "smux: session %d: send frame: queue insert fail", session->m_session_id);
        return -1;
    }

    return 0;
}

uint8_t net_smux_session_can_write(net_smux_session_t session) {
    switch(session->m_underline.m_type) {
    case net_smux_session_underline_udp:
        return session->m_underline.m_udp.m_dgram
            ? 1 : 0;
    case net_smux_session_underline_tcp:
        return (session->m_underline.m_tcp.m_endpoint
                && !net_endpoint_is_writing(session->m_underline.m_tcp.m_endpoint->m_base_endpoint))
            ? 1 : 0;
    }
}

int net_smux_session_write_frame(net_smux_session_t session, net_smux_frame_t frame) {
    net_smux_protocol_t protocol = session->m_protocol;

    if (session->m_underline.m_type == net_smux_session_underline_udp) {
        return 0; 
    }
    else {   
        assert(session->m_underline.m_type == net_smux_session_underline_tcp);

        if (session->m_underline.m_tcp.m_endpoint == NULL) {
            CPE_ERROR(
                protocol->m_em, "smux: session %d: write frame: no endpoint",
                session->m_session_id);
            return -1;
        }

        return 0; 
    }
}

int net_smux_session_send_frame(net_smux_session_t session, net_smux_frame_t frame, uint64_t expire_ms, uint64_t prio) {
    if (net_smux_session_can_write(session) && cpe_priority_queue_count(session->m_shaper) == 0) {
        net_smux_session_write_frame(session, frame);
        net_smux_frame_free(session, frame);
        return 0;
    }
    else {
        return net_smux_session_enqueue_frame(session, frame, expire_ms, prio);
    }
}

int net_smux_write_request_prio_cmp(void const * i_l, void const * i_r) {
    struct net_smux_write_rquest const * l = i_l;
    struct net_smux_write_rquest const * r = i_r;

    return l->m_prio < r->m_prio ? -1
        : l->m_prio > r->m_prio ? 1
        : 0;
}

void net_smux_session_print(write_stream_t ws, net_smux_session_t session) {
    stream_printf(ws, "session %d", session->m_session_id);
}

const char * net_smux_session_dump(mem_buffer_t buffer, net_smux_session_t session) {
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    net_smux_session_print((write_stream_t)&ws, session);
    stream_putc((write_stream_t)&ws, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

int net_smux_session_dgram_eq(net_smux_session_t l, net_smux_session_t r, void * user_data) {
    assert(l->m_underline.m_type == net_smux_session_underline_udp);
    assert(r->m_underline.m_type == net_smux_session_underline_udp);

    return net_address_cmp(
        l->m_underline.m_udp.m_remote_address,
        r->m_underline.m_udp.m_remote_address) == 0 ? 1 : 0;
}

uint32_t net_smux_session_dgram_hash(net_smux_session_t o, void * user_data) {
    assert(o->m_underline.m_type == net_smux_session_underline_udp);
    return net_address_hash(o->m_underline.m_udp.m_remote_address);
}

const char * net_smux_session_runing_mode_str(net_smux_session_runing_mode_t runing_mode) {
    switch (runing_mode) {
    case net_smux_session_runing_mode_cli:
        return "cli";
    case net_smux_session_runing_mode_svr:
        return "svr";
    }
}
