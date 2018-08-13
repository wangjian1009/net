#ifndef NET_NE_DGRAM_SESSION_H_INCLEDED
#define NET_NE_DGRAM_SESSION_H_INCLEDED
#import "NetworkExtension/NWUDPSession.h"
#include "net_ne_dgram.h"

@interface NetNeDgramSessionObserver : NSObject {
    @public net_ne_dgram_session_t m_session;
}
@end

struct net_ne_dgram_session {
    net_ne_dgram_t m_dgram;
    union {
        struct cpe_hash_entry m_hh_for_dgram;
        TAILQ_ENTRY(net_ne_dgram_session) m_next;
    };
    net_address_t m_remote_address;
    __unsafe_unretained NSMutableArray<NSData *> * m_pending;
    __unsafe_unretained NWUDPSession* m_session;
    __unsafe_unretained NetNeDgramSessionObserver * m_observer;
    uint8_t m_writing;
};

net_ne_dgram_session_t net_ne_dgram_session_create(net_ne_dgram_t dgram, net_address_t remote_address);
void net_ne_dgram_session_free(net_ne_dgram_session_t session);
void net_ne_dgram_session_free_all(net_ne_dgram_t dgram);
void net_ne_dgram_session_real_free(net_ne_dgram_session_t session);

net_ne_dgram_session_t net_ne_dgram_session_find(net_ne_dgram_t dgram, net_address_t remote_address);

int net_ne_dgram_session_send(net_ne_dgram_session_t session, void const * data, size_t data_len);

uint32_t net_ne_dgram_session_address_hash(net_ne_dgram_session_t session, void * user_data);
int net_ne_dgram_session_address_eq(net_ne_dgram_session_t l, net_ne_dgram_session_t r, void * user_data);

#endif
