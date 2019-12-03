#ifndef NET_BUFFER_I_H_INCLEDED
#define NET_BUFFER_I_H_INCLEDED
#include "net_buffer.h"

struct net_buffer {
    uint32_t len;
    uint32_t capacity;
    uint8_t* buffer;
    int32_t ref_count;
};

#endif
