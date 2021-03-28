#if CPE_OS_MAC
#include <assert.h>
#include <mach/mach.h>
#include "prometheus_collector.h"
#include "prometheus_collector_metric.h"
#include "prometheus_metric.h"
#include "prometheus_gauge.h"
#include "prometheus_process_collector_i.h"

void prometheus_process_collector_mac_collect(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    assert(provider);

    prometheus_collector_metric_t virtual_memory_bytes =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_virtual_memory_bytes(provider));
    if (virtual_memory_bytes) {
        struct task_basic_info info;
        mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT; //sizeof(info);
        kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
        if (kerr != KERN_SUCCESS) {
        }

        prometheus_metric_t metric = prometheus_collector_metric_metric(virtual_memory_bytes);
        prometheus_gauge_t guage = prometheus_gauge_cast(metric);
        if (prometheus_gauge_set(guage, info.resident_size, NULL) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: %s: set gauge failed",
                prometheus_metric_name(metric));
            return;
        }

        prometheus_collector_metric_set_state(virtual_memory_bytes, prometheus_metric_collected);
    }
}


#endif

