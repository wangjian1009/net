#ifndef NET_NE_DGRAM_SESSION_H_INCLEDED
#define NET_NE_DGRAM_SESSION_H_INCLEDED
#import "NetworkExtension/NWUDPSession.h"
#include "net_ne_dgram.h"

struct net_ne_dgram_session {
    net_ne_dgram_t m_dgram;
    uint32_t m_id;
    struct cpe_hash_entry m_hh_for_id;
    union {
        struct cpe_hash_entry m_hh_for_dgram;
        TAILQ_ENTRY(net_ne_dgram_session) m_next;
    };
    net_address_t m_remote_address;
    __unsafe_unretained NWUDPSession* m_session;
    ringbuffer_block_t m_wb;
};

net_ne_dgram_session_t net_ne_dgram_session_create(net_ne_dgram_t dgram, net_address_t remote_address);
void net_ne_dgram_session_free(net_ne_dgram_session_t session);
void net_ne_dgram_session_free_all(net_ne_dgram_t dgram);
void net_ne_dgram_session_real_free(net_ne_dgram_session_t session);

net_ne_dgram_session_t net_ne_dgram_session_find(net_ne_dgram_t dgram, net_address_t remote_address);

int net_ne_dgram_session_send(net_ne_dgram_session_t session, void const * data, size_t data_len);

uint32_t net_ne_dgram_session_id_hash(net_ne_dgram_session_t session, void * user_data);
int net_ne_dgram_session_id_eq(net_ne_dgram_session_t l, net_ne_dgram_session_t r, void * user_data);

uint32_t net_ne_dgram_session_address_hash(net_ne_dgram_session_t session, void * user_data);
int net_ne_dgram_session_address_eq(net_ne_dgram_session_t l, net_ne_dgram_session_t r, void * user_data);

#endif
