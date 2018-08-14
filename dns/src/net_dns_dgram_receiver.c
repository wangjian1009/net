#include "assert.h"
#include "cpe/pal/pal_types.h"
#include "cpe/pal/pal_string.h"
#include "net_address.h"
#include "net_dns_dgram_receiver_i.h"

net_dns_dgram_receiver_t
net_dns_dgram_receiver_create(
    net_dns_manage_t manage,
    uint8_t address_is_own, net_address_t address,
    void * process_ctx, net_dns_dgram_process_fun_t process_fun)
{
    net_dns_dgram_receiver_t dgram_receiver = mem_alloc(manage->m_alloc, sizeof(struct net_dns_dgram_receiver));
    if (dgram_receiver == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: dgram_receiver alloc fail!");
        return NULL;
    }

    dgram_receiver->m_manage = manage;
    dgram_receiver->m_address_is_own = address_is_own;
    dgram_receiver->m_address = address;
    dgram_receiver->m_process_ctx = process_ctx;
    dgram_receiver->m_process_fun = process_fun;

    cpe_hash_entry_init(&dgram_receiver->m_hh);
    if (cpe_hash_table_insert_unique(&manage->m_dgram_receivers, dgram_receiver) != 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: dgram_receiver of %s duplicate!",
            net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        mem_free(manage->m_alloc, dgram_receiver);
        return NULL;
    }
    
    return dgram_receiver;
}

void net_dns_dgram_receiver_free(net_dns_dgram_receiver_t dgram_receiver) {
    net_dns_manage_t manage = dgram_receiver->m_manage;

    if (dgram_receiver->m_address_is_own) {
        net_address_free(dgram_receiver->m_address);
    }
    dgram_receiver->m_address = NULL;

    cpe_hash_table_remove_by_ins(&manage->m_dgram_receivers, dgram_receiver);

    mem_free(manage->m_alloc, dgram_receiver);
}

void net_dns_dgram_receiver_free_all(net_dns_manage_t manage) {
    struct cpe_hash_it dgram_receiver_it;
    net_dns_dgram_receiver_t dgram_receiver;

    cpe_hash_it_init(&dgram_receiver_it, &manage->m_dgram_receivers);

    dgram_receiver = cpe_hash_it_next(&dgram_receiver_it);
    while(dgram_receiver) {
        net_dns_dgram_receiver_t next = cpe_hash_it_next(&dgram_receiver_it);
        net_dns_dgram_receiver_free(dgram_receiver);
        dgram_receiver = next;
    }
}

net_dns_dgram_receiver_t
net_dns_dgram_receiver_find_by_address(net_dns_manage_t manage, net_address_t address) {
    struct net_dns_dgram_receiver key;
    key.m_address = address;
    return cpe_hash_table_find(&manage->m_dgram_receivers, &key);
}


net_dns_dgram_receiver_t
net_dns_dgram_receiver_find_by_ctx(net_dns_manage_t manage, void * process_ctx, net_dns_dgram_process_fun_t process_fun) {
    struct cpe_hash_it dgram_receiver_it;
    net_dns_dgram_receiver_t dgram_receiver;

    cpe_hash_it_init(&dgram_receiver_it, &manage->m_dgram_receivers);

    while((dgram_receiver = cpe_hash_it_next(&dgram_receiver_it))) {
        if (dgram_receiver->m_process_ctx == process_ctx
            && (process_fun == NULL || process_fun == dgram_receiver->m_process_fun))
        {
            return dgram_receiver;
        }
    }

    return NULL;
}

uint32_t net_dns_dgram_receiver_hash(net_dns_dgram_receiver_t o, void * user_data) {
    return net_address_hash(o->m_address);
}

int net_dns_dgram_receiver_eq(net_dns_dgram_receiver_t l, net_dns_dgram_receiver_t r, void * user_data) {
    return net_address_cmp(l->m_address, r->m_address) == 0 ? 1 : 0;
}
