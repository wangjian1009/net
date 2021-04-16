#ifndef NET_PROGRESS_H_INCLEDED
#define NET_PROGRESS_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

net_progress_t net_progress_create(net_driver_t driver, const char * cmd, const char * mode);
void net_progress_free(net_progress_t progress);

net_driver_t net_progress_driver(net_progress_t progress);

void * net_progress_data(net_progress_t progress);
net_progress_t net_progress_from_data(void * data);

NET_END_DECL

#endif
