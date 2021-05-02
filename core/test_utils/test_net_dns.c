#include "test_memory.h"
#include "test_net_dns.h"
#include "test_net_dns_record.h"
#include "test_net_dns_query.h"
#include "net_schedule.h"

int test_net_dns_local_query(void * ctx, net_address_t hostname, net_address_it_t resolved_it, uint8_t recursive);

test_net_dns_t test_net_dns_create(test_net_driver_t tdriver) {
    test_net_dns_t dns = mem_alloc(test_allocrator(), sizeof(struct test_net_dns));

    dns->m_tdriver = tdriver;
    TAILQ_INIT(&dns->m_records);

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(tdriver));
    
    net_schedule_set_dns_resolver(
        schedule,
        dns, NULL,
        test_net_dns_local_query,
        sizeof(struct test_net_dns_query),
        test_net_dns_query_init,
        test_net_dns_query_fini);

    return dns;
}

void test_net_dns_free(test_net_dns_t dns) {
    while(!TAILQ_EMPTY(&dns->m_records)) {
        test_net_dns_record_free(TAILQ_FIRST(&dns->m_records));
    }

    net_schedule_t schedule = test_net_dns_schedule(dns);
    net_schedule_set_dns_resolver(schedule, NULL, NULL, NULL, 0, NULL, NULL);

    mem_free(test_allocrator(), dns);
}

net_schedule_t test_net_dns_schedule(test_net_dns_t dns) {
    return net_driver_schedule(net_driver_from_data(dns->m_tdriver));
}

int test_net_dns_local_query(void * ctx, net_address_t hostname, net_address_it_t resolved_it, uint8_t recursive) {
    return 0;
}
