#ifndef NET_MEM_BLOCK_I_H_INCLEDED
#define NET_MEM_BLOCK_I_H_INCLEDED
#include "net_mem_block.h"

struct net_mem_block {
    net_schedule_t m_schedule;
    net_endpoint_t m_endpoint;
    net_endpoint_buf_type_t m_buf_type;
    uint32_t len;
    uint32_t capacity;
    uint8_t* buffer;
    int32_t ref_count;
};

#endif
