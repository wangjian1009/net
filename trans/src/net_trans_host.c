#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_http_endpoint.h"
#include "net_trans_host_i.h"
#include "net_trans_http_endpoint_i.h"
#include "net_trans_task_i.h"

net_trans_host_t
net_trans_host_create(net_trans_manage_t mgr, net_address_t address) {
    net_trans_host_t host = NULL;

    host = TAILQ_FIRST(&mgr->m_free_hosts);
    if (host) {
        TAILQ_REMOVE(&mgr->m_free_hosts, host, m_next_for_mgr);
    }
    else {
        host = mem_alloc(mgr->m_alloc, sizeof(struct net_trans_host));
        if (host == NULL) {
            CPE_ERROR(mgr->m_em, "trans: host: alloc fail");
            return NULL;
        }
    }

    host->m_mgr = mgr;
    host->m_endpoint_count = 0;
    TAILQ_INIT(&host->m_endpoints);
    
    host->m_address = net_address_copy(mgr->m_schedule, address);
    if (host->m_address == NULL) {
        CPE_ERROR(
            mgr->m_em, "trans: host: dup address %s fail!",
            net_address_dump(net_trans_manage_tmp_buffer(mgr), address));
        TAILQ_INSERT_TAIL(&mgr->m_free_hosts, host, m_next_for_mgr);
        return NULL;
    }
    
    cpe_hash_entry_init(&host->m_hh_for_mgr);
    if (cpe_hash_table_insert_unique(&mgr->m_hosts, host) != 0) {
        CPE_ERROR(
            mgr->m_em, "trans: host: %s duplicate!",
            net_address_dump(net_trans_manage_tmp_buffer(mgr), host->m_address));
        net_address_free(host->m_address);
        TAILQ_INSERT_TAIL(&mgr->m_free_hosts, host, m_next_for_mgr);
        return NULL;
    }

    return host;
}

void net_trans_host_free(net_trans_host_t host) {
    net_trans_manage_t mgr = host->m_mgr;
    
    while(!TAILQ_EMPTY(&host->m_endpoints)) {
        net_trans_http_endpoint_free(TAILQ_FIRST(&host->m_endpoints));
    }
    assert(host->m_endpoint_count == 0);

    if (host->m_address) {
        net_address_free(host->m_address);
        host->m_address = NULL;
    }
    
    cpe_hash_table_remove_by_ins(&mgr->m_hosts, host);

    TAILQ_INSERT_TAIL(&mgr->m_free_hosts, host, m_next_for_mgr);
}

void net_trans_host_free_all(net_trans_manage_t mgr) {
    struct cpe_hash_it host_it;
    net_trans_host_t host;

    cpe_hash_it_init(&host_it, &mgr->m_hosts);

    host = cpe_hash_it_next(&host_it);
    while(host) {
        net_trans_host_t next = cpe_hash_it_next(&host_it);
        net_trans_host_free(host);
        host = next;
    }
}

void net_trans_host_real_free(net_trans_host_t host) {
    net_trans_manage_t mgr = host->m_mgr;

    TAILQ_REMOVE(&mgr->m_free_hosts, host, m_next_for_mgr);

    mem_free(mgr->m_alloc, host);
}

net_trans_host_t net_trans_host_check_create(net_trans_manage_t mgr, net_address_t address) {
    net_trans_host_t host = net_trans_host_find(mgr, address);
    if (host == NULL) {
        host = net_trans_host_create(mgr, address);
    }
    return host;
}

net_trans_host_t net_trans_host_find(net_trans_manage_t mgr, net_address_t address) {
    struct net_trans_host key;
    key.m_address = address;
    return cpe_hash_table_find(&mgr->m_hosts, &key);
}

net_trans_http_endpoint_t net_trans_host_alloc_endpoint(net_trans_host_t host, uint8_t is_https) {
    net_trans_manage_t mgr = host->m_mgr;
    net_trans_http_endpoint_t trans_http, trans_http_next;
    
    net_trans_http_endpoint_t idle_trans_http = NULL;
    
    for(trans_http = TAILQ_FIRST(&host->m_endpoints);
        trans_http;
        trans_http = trans_http_next)
    {
        trans_http_next = TAILQ_NEXT(trans_http, m_next);

        if (!net_trans_http_endpoint_is_active(trans_http)) {
            return trans_http;
        }

        if (net_trans_http_endpoint_is_https(trans_http) != is_https) continue;
        
        net_trans_task_t last_task = TAILQ_LAST(&trans_http->m_tasks, net_trans_task_list);
        if (last_task && !last_task->m_keep_alive) continue;

        if (idle_trans_http == NULL || trans_http->m_task_count < idle_trans_http->m_task_count) {
            idle_trans_http = trans_http;
        }
    }

    if (idle_trans_http) {
        return idle_trans_http;
    }
    else {
        if (mgr->m_cfg_host_endpoint_limit > 0
            && host->m_endpoint_count >= mgr->m_cfg_host_endpoint_limit)
        {    
            CPE_ERROR(
                mgr->m_em, "trans: host %s connection limit %d reached!",
                net_address_dump(net_trans_manage_tmp_buffer(mgr), host->m_address),
                mgr->m_cfg_host_endpoint_limit);
            return NULL;
        }
        else {
            net_trans_http_endpoint_t new_http_ep = net_trans_http_endpoint_create(host, is_https);
            if (new_http_ep == NULL) {
                CPE_ERROR(
                    mgr->m_em, "trans: host %s create http endpoint fail!",
                    net_address_dump(net_trans_manage_tmp_buffer(mgr), host->m_address));
                return NULL;
            }
            
            return new_http_ep;
        }
    }
}

uint32_t net_trans_host_hash(net_trans_host_t o, void * user_data) {
    return net_address_hash(o->m_address);
}

int net_trans_host_eq(net_trans_host_t l, net_trans_host_t r, void * user_data) {
    return net_address_cmp(l->m_address, r->m_address) == 0 ? 1 : 0;
}
