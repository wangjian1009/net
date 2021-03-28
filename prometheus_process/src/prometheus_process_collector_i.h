#ifndef PROMETHEUS_PROCESS_COLLECTOR_I_H_INCLEDED
#define PROMETHEUS_PROCESS_COLLECTOR_I_H_INCLEDED
#include "prometheus_process_provider_i.h"

struct prometheus_process_collector {
    prometheus_process_provider_t m_provider;
    TAILQ_ENTRY(prometheus_process_collector) m_next;
};

void prometheus_process_collector_free(prometheus_process_collector_t collector);

#if CPE_OS_MAC
void prometheus_process_collector_mac_collect(prometheus_collector_t base_collector);
#endif

void prometheus_process_collector_linux_collect(prometheus_collector_t base_collector);

#endif
