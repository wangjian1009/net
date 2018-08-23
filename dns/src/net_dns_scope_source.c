#include "assert.h"
#include "net_dns_scope_source_i.h"

net_dns_scope_source_t
net_dns_scope_source_create(net_dns_scope_t scope, net_dns_source_t source) {
    net_dns_manage_t manage = scope->m_manage;

    net_dns_scope_source_t scope_source = TAILQ_FIRST(&manage->m_free_scope_sources);
    if (scope_source) {
        TAILQ_REMOVE(&manage->m_free_scope_sources, scope_source, m_next_for_scope);
    }
    else {
        scope_source = mem_alloc(manage->m_alloc, sizeof(struct net_dns_scope_source));
        if (scope_source == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: scope-source alloc fail");
            return NULL;
        }
    }

    scope_source->m_scope = scope;
    scope_source->m_source = source;

    scope->m_source_count++;
    TAILQ_INSERT_TAIL(&scope->m_sources, scope_source, m_next_for_scope);
    TAILQ_INSERT_TAIL(&source->m_scopes, scope_source, m_next_for_source);

    return scope_source;
}

void net_dns_scope_source_free(net_dns_scope_source_t scope_source) {
    net_dns_manage_t manage = scope_source->m_scope->m_manage;

    assert(scope_source->m_scope->m_source_count > 0);
    scope_source->m_scope->m_source_count--;
    TAILQ_REMOVE(&scope_source->m_scope->m_sources, scope_source, m_next_for_scope);
    TAILQ_REMOVE(&scope_source->m_source->m_scopes, scope_source, m_next_for_source);

    scope_source->m_scope = (net_dns_scope_t)manage;
    TAILQ_INSERT_TAIL(&manage->m_free_scope_sources, scope_source, m_next_for_scope);
}

void net_dns_scope_source_real_free(net_dns_scope_source_t scope_source) {
    net_dns_manage_t manage = (net_dns_manage_t)scope_source->m_scope;

    TAILQ_REMOVE(&manage->m_free_scope_sources, scope_source, m_next_for_scope);
    mem_free(manage->m_alloc, scope_source);
}    

