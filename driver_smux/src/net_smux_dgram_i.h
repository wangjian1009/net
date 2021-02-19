#ifndef NET_SMUX_DGRAM_I_H_INCLEDED
#define NET_SMUX_DGRAM_I_H_INCLEDED
#include "cpe/utils/hash.h"
#include "net_smux_dgram.h"
#include "net_smux_protocol_i.h"

struct net_smux_dgram {
    net_smux_protocol_t m_protocol;
    TAILQ_ENTRY(net_smux_dgram) m_next;
    net_smux_runing_mode_t m_runing_mode;
    struct net_smux_config m_config;
    net_smux_mem_cache_t m_mem_cache;
    net_dgram_t m_dgram;
    struct cpe_hash_table m_sessions;
};

#endif
