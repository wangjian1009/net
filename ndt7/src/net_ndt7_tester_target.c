#include "cpe/utils/url.h"
#include "net_ndt7_tester_target_i.h"

net_ndt7_tester_target_t
net_ndt7_tester_target_create(
    net_ndt7_tester_t tester,
    const char * machine, const char * country, const char * city)
{
    net_ndt7_manage_t manager = tester->m_manager;

    net_ndt7_tester_target_t target = mem_alloc(manager->m_alloc, sizeof(struct net_ndt7_tester_target));
    if (target == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: target: alloc fail", tester->m_id);
        return NULL;
    }

    target->m_tester = tester;
    
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(target->m_urls); i++) {
        target->m_urls[i] = NULL;
    }

    TAILQ_INSERT_TAIL(&tester->m_targets, target, m_next);
    
    return target;
}

void net_ndt7_tester_target_free(net_ndt7_tester_target_t target) {
    net_ndt7_tester_t tester = target->m_tester;
    net_ndt7_manage_t manager = tester->m_manager;

    if (target->m_machine) {
        mem_free(manager->m_alloc, target->m_machine);
        target->m_machine = NULL;
    }

    if (target->m_city) {
        mem_free(manager->m_alloc, target->m_city);
        target->m_city = NULL;
    }

    if (target->m_country) {
        mem_free(manager->m_alloc, target->m_country);
        target->m_country = NULL;
    }
    
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(target->m_urls); i++) {
        if (target->m_urls[i]) {
            cpe_url_free(target->m_urls[i]);
            target->m_urls[i] = NULL;
        }
    }
    
    TAILQ_REMOVE(&tester->m_targets, target, m_next);

    mem_free(manager->m_alloc, target);
}

