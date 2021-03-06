#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/priority_queue.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/math_ex.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_dgram.h"
#include "net_timer.h"
#include "net_endpoint.h"
#include "net_smux_session_i.h"
#include "net_smux_stream_i.h"
#include "net_smux_endpoint_i.h"
#include "net_smux_dgram_i.h"
#include "net_smux_frame_i.h"
#include "net_smux_pro.h"

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
    net_smux_runing_mode_t runing_mode,
    net_smux_session_underline_type_t underline_type,
    net_smux_config_t config,
    net_smux_mem_cache_t mem_cache)
{
    net_smux_session_t session = mem_alloc(protocol->m_alloc, sizeof(struct net_smux_session));
    if (session == NULL) {
        CPE_ERROR(protocol->m_em, "smux: session: create: alloc failed");
        return NULL;
    }

    session->m_protocol = protocol;
    session->m_session_id = ++protocol->m_max_session_id;
    session->m_config = config;
    session->m_runing_mode = runing_mode;
    session->m_max_stream_id = runing_mode == net_smux_runing_mode_cli ? 1 : 0;
    session->m_recv_bucket = config->m_max_recv_buffer;
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
    if (!config->m_keep_alive_disabled) {
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
        net_smux_session_create_i(
            protocol, dgram->m_runing_mode, net_smux_session_underline_udp,
            &dgram->m_config, dgram->m_mem_cache);
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
            net_smux_frame_free(session, NULL, request->m_frame);
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

net_smux_runing_mode_t net_smux_session_runing_mode(net_smux_session_t session) {
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
    net_smux_protocol_t protocol = session->m_protocol;
	session->m_max_stream_id += 2;

    net_smux_stream_t stream = net_smux_stream_create(session, session->m_max_stream_id);
    if (stream == NULL) return NULL;

    if (net_smux_session_send_frame(session, stream, net_smux_cmd_syn, NULL, 0, 0, 0) != 0) {
        net_smux_stream_free(stream);
        return NULL;
    }

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
        CPE_ERROR(
            protocol->m_em, "smux: %s: send frame: queue insert fail",
            net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session));
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

void net_smux_session_fill_frame(
    net_smux_session_t session, net_smux_stream_t stream,
    void * output,net_smux_cmd_t cmd, void const * data, uint16_t len)
{
    struct net_smux_head * frame = output;
        
    frame->m_ver = session->m_config->m_version;
    frame->m_cmd = cmd;

    if (stream) {
        CPE_COPY_HTON32(&frame->m_sid, &stream->m_stream_id);
    }
    else {
        frame->m_sid = 0;
    }

    CPE_COPY_HTON16(&frame->m_len, &len);

    if (len) memcpy(frame + 1, data, len);
}
    
int net_smux_session_write_frame(
    net_smux_session_t session, net_smux_stream_t stream,
    net_smux_cmd_t cmd, void const * data, uint16_t len)
{
    net_smux_protocol_t protocol = session->m_protocol;
    uint32_t frame_sz = sizeof(struct net_smux_head) +  len;

    if (session->m_underline.m_type == net_smux_session_underline_udp) {
        mem_buffer_clear_data(&protocol->m_data_buffer);
        void * buf = mem_buffer_alloc(&protocol->m_data_buffer, frame_sz);
        if (buf == NULL) {
            CPE_ERROR(
                protocol->m_em, "smux: %s: write frame: alloc buf error, sz=%d",
                net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session), frame_sz);
            return -1;
        }
        
        net_smux_session_fill_frame(session, stream, buf, cmd, data, len);

        net_dgram_t dgram = session->m_underline.m_udp.m_dgram->m_dgram;
        net_address_t remote_address = session->m_underline.m_udp.m_remote_address;
        
        assert(dgram);
        assert(remote_address);

        if (net_smux_session_debug(session)) {
            char session_name[128];
            cpe_str_dup(
                session_name, sizeof(session_name),
                net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session));
            
            CPE_INFO(
                protocol->m_em, "smux: %s: ==> %s",
                session_name, net_smux_dump_frame(net_smux_protocol_tmp_buffer(protocol), buf));
        }

        if (net_dgram_send(dgram, remote_address, buf, frame_sz) != 0) {
            CPE_ERROR(
                protocol->m_em, "smux: %s: ==> dgram send error",
                net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session));
            return -1;
        }
        
        return 0; 
    }
    else {   
        assert(session->m_underline.m_type == net_smux_session_underline_tcp);
        assert(session->m_underline.m_tcp.m_endpoint != NULL);

        void * buf = net_endpoint_buf_alloc(session->m_underline.m_tcp.m_endpoint->m_base_endpoint, frame_sz);
        if (buf == NULL) {
            CPE_ERROR(
                protocol->m_em, "smux: %s: ==> buf alloc error, len=%d",
                net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session), frame_sz);
            return -1;
        }

        net_smux_session_fill_frame(session, stream, buf, cmd, data, len);

        if (net_endpoint_buf_supply(
                session->m_underline.m_tcp.m_endpoint->m_base_endpoint, net_ep_buf_write, frame_sz) != 0)
        {
            CPE_ERROR(
                protocol->m_em, "smux: %s: ==> supply data error",
                net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session));
            return -1;
        }

        return 0; 
    }
}

uint8_t net_smux_session_debug(net_smux_session_t session) {
    uint8_t d1 = net_protocol_debug(net_protocol_from_data(session->m_protocol));
    uint8_t d2 = 0;
    switch (session->m_underline.m_type) {
    case net_smux_session_underline_udp:
        if (session->m_underline.m_udp.m_dgram) {
            d2 = net_dgram_protocol_debug(
                session->m_underline.m_udp.m_dgram->m_dgram,
                session->m_underline.m_udp.m_remote_address);
        }
        break;
    case net_smux_session_underline_tcp:
        if (session->m_underline.m_tcp.m_endpoint) {
            d2 =  net_endpoint_protocol_debug(session->m_underline.m_tcp.m_endpoint->m_base_endpoint);
        }
        break;
    }

    return cpe_max(d1, d2);
}

int net_smux_session_send_frame(
    net_smux_session_t session, net_smux_stream_t stream,
    net_smux_cmd_t cmd, void const * data, uint16_t len, 
    uint64_t expire_ms, uint64_t prio)
{
    if (net_smux_session_can_write(session) && cpe_priority_queue_count(session->m_shaper) == 0) {
        net_smux_session_write_frame(session, stream, cmd, data, len);
        return 0;
    }
    else {
        assert(0);
        /* net_smux_frame_free( */
        /*     session, */
        /*     frame->m_head.m_sid != 0 ? net_smux_session_find_stream(session, frame->m_head.m_sid) : NULL, */
        /*     frame); */
        //return net_smux_session_enqueue_frame(session, frame, expire_ms, prio);
        return 0;
    }
}

void net_smux_session_get_tokens(net_smux_session_t session, int32_t tokens) {
    uint8_t init_have_token = session->m_recv_bucket > 0 ? 1 : 0;
    
    session->m_recv_bucket -= tokens;

    if (session->m_recv_bucket < 0 && init_have_token) {
        switch(session->m_underline.m_type) {
        case net_smux_session_underline_udp:
            break;
        case net_smux_session_underline_tcp:
            if (session->m_underline.m_tcp.m_endpoint) {
                net_endpoint_set_expect_read(session->m_underline.m_tcp.m_endpoint->m_base_endpoint, 0);
            }
            break;
        }
    }
}

void net_smux_session_return_tokens(net_smux_session_t session, int32_t tokens) {
    uint8_t init_no_token = session->m_recv_bucket < 0 ? 1 : 0;
    
    session->m_recv_bucket += tokens;

    if (session->m_recv_bucket > 0 && init_no_token) {
        switch(session->m_underline.m_type) {
        case net_smux_session_underline_udp:
            break;
        case net_smux_session_underline_tcp:
            if (session->m_underline.m_tcp.m_endpoint) {
                net_endpoint_set_expect_read(session->m_underline.m_tcp.m_endpoint->m_base_endpoint, 1);
            }
            break;
        }
    }
}

int net_smux_session_input(
    net_smux_session_t session, void const * data, uint32_t data_len)
{
    net_smux_protocol_t protocol = session->m_protocol;

    if (data_len < sizeof(struct net_smux_head)) {
        CPE_ERROR(
            protocol->m_em, "smux: %s: <== ignore: data too small, len=%d",
            net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session), data_len);
        return -1;
    }

    struct net_smux_head const * head = data;

    if (head->m_ver != session->m_config->m_version) {
        CPE_ERROR(
            protocol->m_em, "smux: %s: <== ignore: version mismatch, expect %d, input %d",
            net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session),
            session->m_config->m_version, head->m_ver);
        return -1;
    }

    uint16_t pdu_data_len;
    CPE_COPY_NTOH16(&pdu_data_len, &head->m_len);
    uint16_t pdu_len = sizeof(struct net_smux_head) + pdu_data_len;
    
    if (data_len != pdu_len) {
        CPE_ERROR(
            protocol->m_em, "smux: %s: <== ignore: length mismatch pdu-len=%d, input-len=%d",
            net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session),
            pdu_len, data_len);
        return -1;
    }

    uint32_t sid;
    CPE_COPY_NTOH32(&sid, &head->m_sid);

    switch(head->m_cmd) {
    case net_smux_cmd_syn:
        break;
    case net_smux_cmd_fin:
        break;
    case net_smux_cmd_psh:
        break;
    case net_smux_cmd_nop:
        break;
    case net_smux_cmd_udp:
        if (pdu_data_len != sizeof(struct net_smux_body_udp)) {
            CPE_ERROR(
                protocol->m_em, "smux: %s: <== ignore udp cmd: len %d mismatch, expect %d",
                net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session),
                pdu_data_len, (int)sizeof(struct net_smux_body_udp));
            return -1;
        }
        break;
    default:
        CPE_ERROR(
            protocol->m_em, "smux: %s: <== ignore: invalid cmd %d",
            net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session), head->m_cmd);
        return -1;
    }
    
    if (net_smux_session_debug(session)) {
        char session_name[128];
        cpe_str_dup(
            session_name, sizeof(session_name),
            net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session));

        CPE_INFO(
            protocol->m_em, "smux: %s: <== %s",
            session_name, net_smux_dump_frame(net_smux_protocol_tmp_buffer(protocol), data));
    }

    net_smux_stream_t stream = NULL;

    switch((net_smux_cmd_t)head->m_cmd) {
    case net_smux_cmd_syn:
        stream = net_smux_session_find_stream(session, sid);
        if (stream == NULL) {
            stream = net_smux_stream_create(session, sid);
            if (stream == NULL) {
                CPE_ERROR(
                    protocol->m_em, "smux: %s: <== syn: create session fail, sid=%d",
                    net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session), sid);
                return -1;
            }
        }
        break;
    case net_smux_cmd_fin:
        stream = net_smux_session_find_stream(session, sid);
        if (stream != NULL) {
            net_smux_stream_input(stream, NULL, 0);
        }
        break;
    case net_smux_cmd_psh:
        stream = net_smux_session_find_stream(session, sid);
        if (stream != NULL) {
            net_smux_stream_input(stream, head + 1, pdu_data_len);
        }
        break;
    case net_smux_cmd_nop:
        break;
    case net_smux_cmd_udp:
        stream = net_smux_session_find_stream(session, sid);
        if (stream == NULL) {
            CPE_ERROR(
                protocol->m_em, "smux: %s: <== udp: stream %d not exist",
                net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session), sid);
            return -1;
        }
        else {
            struct net_smux_body_udp const * body = (void*)(head + 1);

            uint32_t consumed;
            CPE_COPY_NTOH32(&consumed, &body->m_consumed);
            
            uint32_t window;
            CPE_COPY_NTOH32(&window, &body->m_window);

            net_smux_stream_update_pear(stream, consumed, window);
        }
        break;
    }
    
    return 0;
}

int net_smux_write_request_prio_cmp(void const * i_l, void const * i_r) {
    struct net_smux_write_rquest const * l = i_l;
    struct net_smux_write_rquest const * r = i_r;

    return l->m_prio < r->m_prio ? -1
        : l->m_prio > r->m_prio ? 1
        : 0;
}

void net_smux_session_print(write_stream_t ws, net_smux_session_t session) {
    switch(session->m_underline.m_type) {
    case net_smux_session_underline_udp:
        if (session->m_underline.m_udp.m_dgram) {
            net_smux_dgram_print(ws, session->m_underline.m_udp.m_dgram);
            stream_printf(ws, ": ");
        }
        break;
    case net_smux_session_underline_tcp:
        break;
    }
    
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
