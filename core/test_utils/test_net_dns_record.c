#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "net_address.h"
#include "test_memory.h"
#include "test_net_dns_record.h"

test_net_dns_record_t
test_net_dns_record_create(test_net_dns_t dns, net_address_t hostname) {
    test_net_dns_record_t record =
        mem_alloc(test_allocrator(), sizeof(struct test_net_dns_record));
    assert_true(record);

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(dns->m_tdriver));
    
    record->m_dns = dns;
    record->m_hostname = net_address_copy(schedule, hostname);
    assert_true(record->m_hostname);

    record->m_resolved_count = 0;
    record->m_resolved_capacity = 0;
    record->m_resolveds = NULL;

    TAILQ_INSERT_TAIL(&dns->m_records, record, m_next_for_dns);

    return record;
}

void test_net_dns_record_free(test_net_dns_record_t record) {
    test_net_dns_t dns = record->m_dns;

    uint16_t i;
    for(i = 0; i < record->m_resolved_count; i++) {
        net_address_free(record->m_resolveds[i]);
    };
    record->m_resolved_count = 0;

    if (record->m_resolveds) {
        mem_free(test_allocrator(), record->m_resolveds);
        record->m_resolveds = NULL;
        record->m_resolved_capacity = 0;
    }
    
    net_address_free(record->m_hostname);
    record->m_hostname = NULL;
    
    TAILQ_REMOVE(&dns->m_records, record, m_next_for_dns);
    mem_free(test_allocrator(), record);
}

uint8_t test_net_dns_record_ins_in_resolved(test_net_dns_record_t record, net_address_t resolved) {
    uint16_t i;

    for(i = 0; i < record->m_resolved_count; i++) {
        if (net_address_cmp(record->m_resolveds[i], resolved) == 0) return 1;
    };

    return 0;
}

int test_net_dns_record_add_resolved(test_net_dns_record_t record, net_address_t resolved) {
    if (record->m_resolved_count >= record->m_resolved_capacity) {
        uint16_t new_capacity = record->m_resolved_capacity < 8 ? 8 : record->m_resolved_capacity * 2;
        net_address_t * new_resolveds = mem_alloc(test_allocrator(), sizeof(net_address_t) * new_capacity);

        if (record->m_resolveds) {
            memcpy(new_resolveds, record->m_resolveds, sizeof(net_address_t) * record->m_resolved_count);
            mem_free(test_allocrator(), record->m_resolveds);
        }

        record->m_resolved_capacity = new_capacity;
        record->m_resolveds = new_resolveds;
    }

    net_schedule_t schedule = test_net_dns_schedule(record->m_dns);
    record->m_resolveds[record->m_resolved_count] = net_address_copy(schedule, resolved);
    record->m_resolved_count++;

    return 0;
}

struct test_net_dns_record_resolveds_it_data {
    test_net_dns_record_t m_record;
    uint16_t m_pos;
};

static net_address_t test_net_dns_record_resolved_it_next(net_address_it_t it) {
    struct test_net_dns_record_resolveds_it_data * it_data = (void*)it->data;

    net_address_t r = NULL;
    if (it_data->m_pos < it_data->m_record->m_resolved_count) {
        r = it_data->m_record->m_resolveds[it_data->m_pos];
        it_data->m_pos++;
    }
    
    return r;
}

void test_net_dns_record_resolveds(test_net_dns_record_t record, net_address_it_t it) {
    struct test_net_dns_record_resolveds_it_data * it_data = (void*)it->data;
    it_data->m_record = record;
    it_data->m_pos = 0;
    it->next = test_net_dns_record_resolved_it_next;
}
