#include "assert.h"
#include "cpe/pal/pal_types.h"
#include "cpe/pal/pal_string.h"
#include "net_address.h"
#include "net_dns_scope_i.h"

net_dns_scope_t
net_dns_scope_create(net_dns_manage_t manage, const char * name) {
    size_t name_len = strlen(name) + 1;

    net_dns_scope_t scope = mem_alloc(manage->m_alloc, sizeof(struct net_dns_scope) + name_len);
    if (scope == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: scope alloc fail!");
        return NULL;
    }

    scope->m_manage = manage;
    scope->m_name = (void*)(scope + 1);
    memcpy((void*)scope->m_name, name, name_len);
    
    cpe_hash_entry_init(&scope->m_hh);
    if (cpe_hash_table_insert_unique(&manage->m_scopes, scope) != 0) {
        CPE_ERROR(manage->m_em, "dns-cli: scope %s duplicate!", name);
        mem_free(manage->m_alloc, scope);
        return NULL;
    }

    return scope;
}

void net_dns_scope_free(net_dns_scope_t scope) {
    net_dns_manage_t manage = scope->m_manage;

    cpe_hash_table_remove_by_ins(&manage->m_scopes, scope);

    mem_free(manage->m_alloc, scope);
}

void net_dns_scope_free_all(net_dns_manage_t manage) {
    struct cpe_hash_it scope_it;
    net_dns_scope_t scope;

    cpe_hash_it_init(&scope_it, &manage->m_entries);

    scope = cpe_hash_it_next(&scope_it);
    while(scope) {
        net_dns_scope_t next = cpe_hash_it_next(&scope_it);
        net_dns_scope_free(scope);
        scope = next;
    }
}

const char * net_dns_scope_name(net_dns_scope_t scope) {
    return scope->m_name;
}

net_dns_scope_t
net_dns_scope_find(net_dns_manage_t manage, const char * name) {
    struct net_dns_scope key;
    key.m_name = name;
    return cpe_hash_table_find(&manage->m_entries, &key);
}

static net_dns_scope_t net_dns_scope_cname_it_next(net_dns_scope_it_t it) {
    struct cpe_hash_it * scope_it = (struct cpe_hash_it *)it->data;
    return cpe_hash_it_next(scope_it);
}

void net_dns_scopes(net_dns_manage_t manage, net_dns_scope_it_t it) {
    struct cpe_hash_it * scope_it = (struct cpe_hash_it *)it->data;
    cpe_hash_it_init(scope_it, &manage->m_scopes);
    it->next = net_dns_scope_cname_it_next;
}

uint32_t net_dns_scope_hash(net_dns_scope_t o, void * user_data) {
    return cpe_hash_str(o->m_name, strlen(o->m_name));
}

int net_dns_scope_eq(net_dns_scope_t l, net_dns_scope_t r, void * user_data) {
    return strcmp(l->m_name, r->m_name) == 0 ? 1 : 0;
}
