#ifndef NET_NDT7_TESTER_I_H_INCLEDED
#define NET_NDT7_TESTER_I_H_INCLEDED
#include "net_ndt7_tester.h"
#include "net_ndt7_manage_i.h"

NET_BEGIN_DECL

struct net_ndt7_tester {
    net_ndt7_manage_t m_manager;
    TAILQ_ENTRY(net_ndt7_tester) m_next;
    
    net_ndt7_test_type_t m_type;
    net_ndt7_tester_state_t m_state;
};

NET_END_DECL

#endif
