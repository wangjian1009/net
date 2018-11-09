#ifndef NET_STATISTICS_REPORTOR_TYPE_I_H_INCLEDED
#define NET_STATISTICS_REPORTOR_TYPE_I_H_INCLEDED
#include "net_statistics_reportor_type.h"
#include "net_statistics_i.h"

CPE_BEGIN_DECL

struct net_statistics_reportor_type {
    net_statistics_t m_statistics;
    TAILQ_ENTRY(net_statistics_reportor_type) m_next;
    char m_name[32];
    void * m_ctx;
    uint16_t m_capacity;
    net_statistics_reportor_init_fun_t m_init_fun;
    net_statistics_reportor_fini_fun_t m_fini_fun;

    net_statistics_reportor_list_t m_reportors;
};

CPE_END_DECL

#endif

