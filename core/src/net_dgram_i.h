#ifndef NET_DGRAM_I_H_INCLEDED
#define NET_DGRAM_I_H_INCLEDED
#include "net_dgram.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_dgram {
    net_driver_t m_driver;
    TAILQ_ENTRY(net_dgram) m_next_for_driver;
    uint32_t m_id;
    uint8_t m_is_processing;
    uint8_t m_is_free;
    uint8_t m_driver_debug;
    net_address_t m_address;
    net_dgram_process_fun_t m_process_fun;
    void * m_process_ctx;
};

void net_dgram_real_free(net_dgram_t dgram);

NET_END_DECL

#endif
