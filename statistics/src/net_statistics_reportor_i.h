#ifndef NET_STATISTICS_REPORTOR_I_H_INCLEDED
#define NET_STATISTICS_REPORTOR_I_H_INCLEDED
#include "net_statistics_reportor.h"
#include "net_statistics_i.h"

CPE_BEGIN_DECL

struct net_statistics_reportor {
    net_statistics_t m_statistics;
    TAILQ_ENTRY(net_statistics_reportor) m_next_for_statistics;
    net_statistics_reportor_type_t m_type;
    TAILQ_ENTRY(net_statistics_reportor) m_next_for_type;
};

CPE_END_DECL

#endif

