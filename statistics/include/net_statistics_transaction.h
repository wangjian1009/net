#ifndef NET_STATISTICS_TRANSACTION_H_INCLEDED
#define NET_STATISTICS_TRANSACTION_H_INCLEDED
#include "net_statistics_system.h"

CPE_BEGIN_DECL

net_statistics_transaction_t net_statistics_transaction_create(net_statistics_t statistics, const char * type);
void net_statistics_transaction_free(net_statistics_transaction_t transaction);

void net_statistics_transaction_set_state(net_statistics_transaction_t transaction, const char * state);

CPE_END_DECL

#endif
