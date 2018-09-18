#ifndef NET_TRANS_HOST_I_H_INCLEDED
#define NET_TRANS_HOST_I_H_INCLEDED
#include "net_trans_manage_i.h"

NET_BEGIN_DECL

struct net_trans_host {
    net_trans_manage_t m_mgr;
    net_address_t m_address;
    cpe_hash_entry m_hh;
    net_trans_http_endpoint_list_t m_endpoints;
    net_trans_task_list_t m_tasks;
};

net_trans_host_t net_trans_host_create(net_trans_manage_t mgr, net_address_t address);
void net_trans_host_free(net_trans_host_t host);
void net_trans_host_real_free(net_trans_host_t host);

net_trans_host_t net_trans_host_check_create(net_trans_manage_t mgr, net_address_t address);

net_trans_http_endpoint_t net_trans_host_alloc_endpoint(net_trans_host_t host);

NET_END_DECL

#endif
