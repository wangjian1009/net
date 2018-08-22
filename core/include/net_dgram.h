#ifndef NET_DGRAM_H_INCLEDED
#define NET_DGRAM_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef void (*net_dgram_process_fun_t)(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_dgram_t net_dgram_create(
    net_driver_t driver,
    net_address_t address,
    net_dgram_process_fun_t process_fun, void * process_ctx);

net_dgram_t net_dgram_auto_create(
    net_schedule_t schedule,
    net_address_t address,
    net_dgram_process_fun_t process_fun, void * process_ctx);

void net_dgram_free(net_dgram_t dgram);


net_schedule_t net_dgram_schedule(net_dgram_t dgram);
net_driver_t net_dgram_driver(net_dgram_t dgram);

net_address_t net_dgram_address(net_dgram_t dgram);
void net_dgram_set_address(net_dgram_t dgram, net_address_t address);

void * net_dgram_data(net_dgram_t dgram);
net_dgram_t net_dgram_from_data(void * data);

uint8_t net_dgram_protocol_debug(net_dgram_t dgram, net_address_t remote);

uint8_t net_dgram_driver_debug(net_dgram_t dgram);
void net_dgram_set_driver_debug(net_dgram_t dgram, uint8_t debug);

int net_dgram_send(net_dgram_t dgram, net_address_t target, void const * data, size_t data_size);
void net_dgram_recv(net_dgram_t dgram, net_address_t from, void * data, size_t data_size);

NET_END_DECL

#endif
