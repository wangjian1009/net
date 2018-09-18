#ifndef NET_TRANS_HOST_I_H_INCLEDED
#define NET_TRANS_HOST_I_H_INCLEDED
#include "net_trans_manage_i.h"

NET_BEGIN_DECL

struct net_trans_host {
    net_trans_manage_t m_mgr;
    union {
        struct cpe_hash_entry m_hh_for_mgr;
        TAILQ_ENTRY(net_trans_host) m_next_for_mgr;
    };
    net_address_t m_address;
    uint16_t m_endpoint_count;
    net_trans_http_endpoint_list_t m_endpoints;
    net_trans_task_list_t m_tasks;
};

net_trans_host_t net_trans_host_create(net_trans_manage_t mgr, net_address_t address);
void net_trans_host_free(net_trans_host_t host);
void net_trans_host_free_all(net_trans_manage_t mgr);
void net_trans_host_real_free(net_trans_host_t host);

net_trans_host_t net_trans_host_find(net_trans_manage_t mgr, net_address_t address);
net_trans_host_t net_trans_host_check_create(net_trans_manage_t mgr, net_address_t address);

net_trans_http_endpoint_t net_trans_host_alloc_endpoint(net_trans_host_t host);

uint32_t net_trans_host_hash(net_trans_host_t o, void * user_data);
int net_trans_host_eq(net_trans_host_t l, net_trans_host_t r, void * user_data);

NET_END_DECL

#endif
