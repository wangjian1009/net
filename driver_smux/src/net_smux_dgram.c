#include "cpe/utils/stream_buffer.h"
#include "net_dgram.h"
#include "net_address.h"
#include "net_smux_dgram_i.h"
#include "net_smux_session_i.h"

void net_smux_dgram_input(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_smux_dgram_t
net_smux_dgram_create(
    net_smux_protocol_t protocol,
    net_smux_session_runing_mode_t runing_mode, net_driver_t driver, net_address_t local_address)
{
    net_smux_dgram_t dgram = mem_alloc(protocol->m_alloc, sizeof(struct net_smux_dgram));
    if (dgram == NULL) {
        CPE_ERROR(
            protocol->m_em, "smux: dgram %s: create: alloc fail!",
            net_address_dump(net_smux_protocol_tmp_buffer(protocol), local_address));
        return NULL;
    }

    dgram->m_protocol = protocol;

    dgram->m_dgram = net_dgram_create(driver, local_address, net_smux_dgram_input, dgram);
    if (dgram->m_dgram == NULL) {
        CPE_ERROR(
            protocol->m_em, "smux: dgram %s: create: create net dgram fail!",
            net_address_dump(net_smux_protocol_tmp_buffer(protocol), local_address));
        mem_free(protocol->m_alloc, dgram);
        return NULL;
    }
        
    if (cpe_hash_table_init(
            &dgram->m_sessions,
            protocol->m_alloc,
            (cpe_hash_fun_t) net_smux_session_dgram_hash,
            (cpe_hash_eq_t) net_smux_session_dgram_eq,
            CPE_HASH_OBJ2ENTRY(net_smux_session, m_underline.m_udp.m_hh_for_dgram),
            -1) != 0)
    {
        CPE_ERROR(
            protocol->m_em, "smux: dgram %s: create: init hash table fail!",
            net_address_dump(net_smux_protocol_tmp_buffer(protocol), local_address));
        net_dgram_free(dgram->m_dgram);
        mem_free(protocol->m_alloc, dgram);
        return NULL;
    }

    TAILQ_INSERT_TAIL(&protocol->m_dgrams, dgram, m_next);
    return dgram;
}

void net_smux_dgram_free(net_smux_dgram_t dgram) {
    net_smux_protocol_t protocol = dgram->m_protocol;

    struct cpe_hash_it session_it;
    net_smux_session_t session;
    cpe_hash_it_init(&session_it, &dgram->m_sessions);
    session = cpe_hash_it_next(&session_it);
    while(session) {
        net_smux_session_t next = cpe_hash_it_next(&session_it);
        net_smux_session_free(session);
        session = next;
    }
    cpe_hash_table_fini(&dgram->m_sessions);

    if (dgram->m_dgram) {
        net_dgram_free(dgram->m_dgram);
        dgram->m_dgram = NULL;
    }

    TAILQ_REMOVE(&protocol->m_dgrams, dgram, m_next);
    mem_free(protocol->m_alloc, dgram);
}

net_smux_session_t
net_smux_dgram_find_session(net_smux_dgram_t dgram, net_address_t remote_address) {
    struct net_smux_session key;
    key.m_underline.m_type = net_smux_session_underline_udp;
    key.m_underline.m_udp.m_remote_address = remote_address;

    return cpe_hash_table_find(&dgram->m_sessions, &key);
}

net_smux_session_t
net_smux_dgram_open_session(net_smux_dgram_t dgram, net_address_t remote_address) {
    net_smux_protocol_t protocol = dgram->m_protocol;
    
    if (dgram->m_runing_mode != net_smux_session_runing_mode_cli) {
        //CPE_ERROR(protocol->m_em, "smux: dg %d:"
    }
    
    return net_smux_session_create_dgram(dgram->m_protocol, dgram, remote_address);
}

net_smux_session_t
net_smux_dgram_check_open_session(net_smux_dgram_t dgram, net_address_t remote_address) {
    net_smux_session_t session = net_smux_dgram_find_session(dgram, remote_address);
    if (session == NULL) {
        session = net_smux_dgram_open_session(dgram, remote_address);
    }

    return session;
}

void net_smux_dgram_print(write_stream_t ws, net_smux_dgram_t dgram) {
    stream_printf(ws, "dgram ");
    net_address_print(ws, net_dgram_address(dgram->m_dgram));
}

const char * net_smux_dgram_dump(mem_buffer_t buffer, net_smux_dgram_t dgram) {
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    net_smux_dgram_print((write_stream_t)&ws, dgram);
    stream_putc((write_stream_t)&ws, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

void net_smux_dgram_input(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source) {
}
