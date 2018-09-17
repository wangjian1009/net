#ifndef NET_TRANS_HOST_I_H_INCLEDED
#define NET_TRANS_HOST_I_H_INCLEDED
#include "net_trans_manage_i.h"

NET_BEGIN_DECL

struct net_trans_host {
    net_trans_manage_t m_manage;
    net_address_t m_host;
    cpe_hash_entry m_hh;
    net_trans_http_endpoint_list_t m_endpoints;
    net_trans_task_list_t m_tasks;
};

NET_END_DECL

#endif
