#ifndef NET_ICMP_TYPES_H_INCLEDED
#define NET_ICMP_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_icmp_mgr * net_icmp_mgr_t;
typedef struct net_icmp_ping_task * net_icmp_ping_task_t;
typedef struct net_icmp_ping_record * net_icmp_ping_record_t;
typedef struct net_icmp_ping_record_it * net_icmp_ping_record_it_t;

typedef enum net_icmp_ping_task_state net_icmp_ping_task_state_t;

NET_END_DECL

#endif
