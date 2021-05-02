#ifndef NET_NDT7_TESTER_TARGET_I_H_INCLEDED
#define NET_NDT7_TESTER_TARGET_I_H_INCLEDED
#include "net_ndt7_tester_target.h"
#include "net_ndt7_tester_i.h"

NET_BEGIN_DECL

struct net_ndt7_tester_target {
    net_ndt7_tester_t m_tester;
    TAILQ_ENTRY(net_ndt7_tester_target) m_next;
    char * m_machine;
    char * m_city;
    char * m_country;
    cpe_url_t m_urls[net_ndt7_target_url_category_count];
};

net_ndt7_tester_target_t
net_ndt7_tester_target_create(
    net_ndt7_tester_t tester,
    const char * machine, const char * country, const char * city);

void net_ndt7_tester_target_free(net_ndt7_tester_target_t target);

NET_END_DECL

#endif
