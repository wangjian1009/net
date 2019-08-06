#include "net_schedule.h"
#include "with_dns_manager.h"

with_dns_manager_t with_dns_manager = NULL;

void with_dns_manager_setup(void) {
    ck_assert(with_dns_manager == NULL);
    
    with_dns_manager = mem_alloc(NULL, sizeof(struct with_dns_manager));
    ck_assert(with_dns_manager != NULL);
    
    with_dns_manager->m_alloc = NULL;
    cpe_error_monitor_init(&with_dns_manager->m_em_buf, cpe_error_log_to_consol, NULL);

    with_dns_manager->m_schedule = net_schedule_create(
        with_dns_manager->m_alloc, &with_dns_manager->m_em_buf, 5 * 1024 * 1025);
    ck_assert(with_dns_manager->m_schedule);
    
    with_dns_manager->m_dns = 
        net_dns_manage_create(with_dns_manager->m_alloc, &with_dns_manager->m_em_buf, with_dns_manager->m_schedule);
    ck_assert(with_dns_manager->m_dns);
}

void with_dns_manager_teardown(void) {
    ck_assert(with_dns_manager != NULL);

    net_dns_manage_free(with_dns_manager->m_dns);
    net_schedule_free(with_dns_manager->m_schedule);
    mem_free(with_dns_manager->m_alloc, with_dns_manager);
    with_dns_manager = NULL;
}
