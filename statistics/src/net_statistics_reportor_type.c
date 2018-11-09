#include "cpe/utils/string_utils.h"
#include "net_statistics_reportor_type_i.h"
#include "net_statistics_reportor_i.h"

net_statistics_reportor_type_t
net_statistics_reportor_type_create(
    net_statistics_t statistics,
    const char * name,
    void * ctx,
    uint16_t capacity,
    net_statistics_reportor_init_fun_t init_fun,
    net_statistics_reportor_fini_fun_t fini_fun)
{
    net_statistics_reportor_type_t reportor_type = mem_alloc(statistics->m_alloc, sizeof(struct net_statistics_reportor_type));
    if (reportor_type == NULL) {
        CPE_ERROR(statistics->m_em, "statistics: type %s: alloc fail", name);
        return NULL;
    }

    reportor_type->m_statistics = statistics;
    cpe_str_dup(reportor_type->m_name, sizeof(reportor_type->m_name), name);
    reportor_type->m_ctx = ctx;
    reportor_type->m_capacity = capacity;
    reportor_type->m_init_fun = init_fun;
    reportor_type->m_fini_fun = fini_fun;

    TAILQ_INIT(&reportor_type->m_reportors);

    TAILQ_INSERT_TAIL(&statistics->m_reportor_types, reportor_type, m_next);

    return reportor_type;
}

void net_statistics_reportor_type_free(net_statistics_reportor_type_t reportor_type) {
    net_statistics_t statistics = reportor_type->m_statistics;

    while(!TAILQ_EMPTY(&reportor_type->m_reportors)) {
        net_statistics_reportor_free(TAILQ_FIRST(&reportor_type->m_reportors));
    }

    TAILQ_INSERT_TAIL(&statistics->m_reportor_types, reportor_type, m_next);

    mem_free(statistics->m_alloc, reportor_type);
}

