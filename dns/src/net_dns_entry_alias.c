#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_dns_entry_alias_i.h"
#include "net_dns_source_i.h"

net_dns_entry_alias_t
net_dns_entry_alias_create(net_dns_entry_t origin, net_dns_entry_t as) {
    net_dns_manage_t manage = origin->m_manage;

    net_dns_entry_alias_t alias = TAILQ_FIRST(&manage->m_free_entry_aliass);
    if (alias) {
        TAILQ_REMOVE(&manage->m_free_entry_aliass, alias, m_next_for_origin);
    }
    else {
        alias = mem_alloc(manage->m_alloc, sizeof(struct net_dns_entry_alias));
        if (alias == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: entry alias alloc fail");
            return NULL;
        }
    }

    alias->m_origin = origin;
    alias->m_as = as;

    TAILQ_INSERT_TAIL(&origin->m_cnames, alias, m_next_for_origin);
    TAILQ_INSERT_TAIL(&as->m_origins, alias, m_next_for_as);

    return alias;
}
        
void net_dns_entry_alias_free(net_dns_entry_alias_t alias) {
    net_dns_manage_t manage = alias->m_origin->m_manage;

    TAILQ_REMOVE(&alias->m_origin->m_cnames, alias, m_next_for_origin);
    TAILQ_REMOVE(&alias->m_as->m_origins, alias, m_next_for_as);
    
    alias->m_origin = (net_dns_entry_t)manage;
    TAILQ_INSERT_TAIL(&manage->m_free_entry_aliass, alias, m_next_for_origin);
}

void net_dns_entry_alias_real_free(net_dns_entry_alias_t alias) {
    net_dns_manage_t manage = (net_dns_manage_t)alias->m_origin;
    TAILQ_REMOVE(&manage->m_free_entry_aliass, alias, m_next_for_origin);
    mem_free(manage->m_alloc, alias);
}
