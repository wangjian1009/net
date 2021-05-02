#include "cmocka_all.h"
#include "net_address.h"
#include "test_memory.h"
#include "test_net_dns_record.h"

test_net_dns_record_t
test_net_dns_record_create(test_net_dns_t dns, net_address_t target) {
    test_net_dns_record_t record =
        mem_alloc(test_allocrator(), sizeof(struct test_net_dns_record));
    assert_true(record);

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(dns->m_tdriver));
    
    record->m_dns = dns;
    record->m_target = net_address_copy(schedule, target);
    assert_true(record->m_target);

    TAILQ_INSERT_TAIL(&dns->m_records, record, m_next_for_dns);

    return record;
}

void test_net_dns_record_free(test_net_dns_record_t record) {
    test_net_dns_t dns = record->m_dns;

    net_address_free(record->m_target);
    record->m_target = NULL;
    
    TAILQ_REMOVE(&dns->m_records, record, m_next_for_dns);
    mem_free(test_allocrator(), record);
}

