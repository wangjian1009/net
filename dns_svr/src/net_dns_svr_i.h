#ifndef NET_DNS_SVR_I_H_INCLEDED
#define NET_DNS_SVR_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_dns_svr.h"

NET_BEGIN_DECL

typedef struct net_dns_svr_protocol * net_dns_svr_protocol_t;
typedef struct net_dns_svr_endpoint * net_dns_svr_endpoint_t;
typedef struct net_dns_svr_query_entry * net_dns_svr_query_entry_t;

typedef TAILQ_HEAD(net_dns_svr_monitor_list, net_dns_svr_monitor) net_dns_svr_monitor_list_t;
typedef TAILQ_HEAD(net_dns_svr_itf_list, net_dns_svr_itf) net_dns_svr_itf_list_t;
typedef TAILQ_HEAD(net_dns_svr_query_list, net_dns_svr_query) net_dns_svr_query_list_t;
typedef TAILQ_HEAD(net_dns_svr_query_entry_list, net_dns_svr_query_entry) net_dns_svr_query_entry_list_t;

struct net_dns_svr {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    uint8_t m_debug;
    net_protocol_t m_dns_protocol;
    net_dns_svr_monitor_list_t m_monitors;
    net_dns_svr_itf_list_t m_itfs;

    struct mem_buffer m_data_buffer;
    
    net_dns_svr_query_list_t m_free_querys;
    net_dns_svr_query_entry_list_t m_free_query_entries;
};

mem_buffer_t net_dns_svr_tmp_buffer(net_dns_svr_t dns_svr);

NET_END_DECL

#endif


