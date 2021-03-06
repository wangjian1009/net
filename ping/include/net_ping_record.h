#ifndef NET_PING_RECORD_H_INCLEDED
#define NET_PING_RECORD_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ping_types.h"

NET_BEGIN_DECL

struct net_ping_record_it {
    net_ping_record_t (*next)(net_ping_record_it_t it);
    char data[64];
};

net_ping_error_t net_ping_record_error(net_ping_record_t record);
uint32_t net_ping_record_value(net_ping_record_t record);

void net_ping_record_print(write_stream_t ws, net_ping_record_t record);
const char * net_ping_record_dump(mem_buffer_t buffer, net_ping_record_t record);

#define net_ping_record_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
