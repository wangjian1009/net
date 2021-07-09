#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_unistd.h"
#include "prometheus_collector.h"
#include "prometheus_collector_metric.h"
#include "prometheus_metric.h"
#include "prometheus_gauge.h"
#include "prometheus_counter.h"
#include "prometheus_process_collector_i.h"
#include "prometheus_process_linux_limits_i.h"
#include "prometheus_process_linux_fds_i.h"
#include "prometheus_process_linux_stat_i.h"

/*limit*/
struct prometheus_process_collector_limits_ctx {
    prometheus_process_collector_t m_collector;
    prometheus_collector_metric_t m_max_fds;
    prometheus_collector_metric_t m_virtual_memory_max_bytes;
};

void prometheus_process_collector_on_row(void * i_ctx, prometheus_process_linux_limits_row_t row) {
    struct prometheus_process_collector_limits_ctx * ctx = i_ctx;
    prometheus_process_provider_t provider = ctx->m_collector->m_provider;

    if (strcmp(row->limit, "Max open files") == 0) {
        if (ctx->m_max_fds == NULL) return;

        prometheus_gauge_t guage = prometheus_gauge_cast(prometheus_collector_metric_metric(ctx->m_max_fds));
        if (prometheus_gauge_set(guage, row->soft, NULL) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: limit.%s: set gauge failed", row->limit);
            return;
        }

        prometheus_collector_metric_set_state(ctx->m_max_fds, prometheus_metric_collected);
    }
    else if (strcmp(row->limit, "Max address space") == 0) {
        if (ctx->m_virtual_memory_max_bytes == NULL) return;

        prometheus_gauge_t guage = prometheus_gauge_cast(prometheus_collector_metric_metric(ctx->m_virtual_memory_max_bytes));
        if (prometheus_gauge_set(guage, row->soft, NULL) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: limit.%s: set gauge failed", row->limit);
            return;
        }

        prometheus_collector_metric_set_state(ctx->m_virtual_memory_max_bytes, prometheus_metric_collected);
    }
}

void prometheus_process_collector_from_limits(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    assert(provider);

    struct prometheus_process_collector_limits_ctx limit_ctx = {
        collector,
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_max_fds(provider)),
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_virtual_memory_max_bytes(provider)),
    };

    if (limit_ctx.m_max_fds == NULL
        && limit_ctx.m_virtual_memory_max_bytes == NULL
        )
    {
        return;
    }

    if (prometheus_process_linux_limits_load(provider, &limit_ctx, prometheus_process_collector_on_row) != 0) {
        CPE_ERROR(provider->m_em, "prometheus: process: collect: process limit fail");
    }
}

void prometheus_process_collector_from_state(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    assert(provider);

    prometheus_collector_metric_t cpu_seconds_total =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_cpu_seconds_total(provider));

    prometheus_collector_metric_t virtual_memory_bytes =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_virtual_memory_bytes(provider));
    
    prometheus_collector_metric_t resident_memory_bytes =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_resident_memory_bytes(provider));
    
    prometheus_collector_metric_t start_time_seconds =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_start_time_seconds(provider));

    if (cpu_seconds_total == NULL
        && virtual_memory_bytes == NULL
        && resident_memory_bytes == NULL
        && start_time_seconds == NULL) return;

    struct prometheus_process_linux_stat process_stat;
    prometheus_process_linux_stat_init(&process_stat);

    if (prometheus_process_linux_stat_load(provider, &process_stat) != 0) {
        CPE_ERROR(provider->m_em, "prometheus: process: collect: process stat fail");
        return;
    }

    if (cpu_seconds_total != NULL) {
        prometheus_metric_t metric = prometheus_collector_metric_metric(cpu_seconds_total);
        prometheus_counter_t counter = prometheus_counter_cast(metric);
        double value = (double)(process_stat.utime + process_stat.stime) / (double)sysconf(_SC_CLK_TCK);
        if (prometheus_counter_set(counter, value, NULL) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: %s: set counter failed", prometheus_metric_name(metric));
        }
        else {
            prometheus_collector_metric_set_state(cpu_seconds_total, prometheus_metric_collected);
        }
    }

    if (virtual_memory_bytes != NULL) {
        prometheus_metric_t metric = prometheus_collector_metric_metric(virtual_memory_bytes);
        prometheus_gauge_t guage = prometheus_gauge_cast(metric);
        if (prometheus_gauge_set(guage, process_stat.vsize, NULL) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: %s: set gauge failed", prometheus_metric_name(metric));
        }
        else {
            prometheus_collector_metric_set_state(virtual_memory_bytes, prometheus_metric_collected);
        }
    }

    if (resident_memory_bytes != NULL) {
        prometheus_metric_t metric = prometheus_collector_metric_metric(resident_memory_bytes);
        prometheus_gauge_t guage = prometheus_gauge_cast(metric);
        if (prometheus_gauge_set(guage, process_stat.rss * sysconf(_SC_PAGE_SIZE), NULL) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: %s: set gauge failed", prometheus_metric_name(metric));
        }
        else {
            prometheus_collector_metric_set_state(resident_memory_bytes, prometheus_metric_collected);
        }
    }

    if (start_time_seconds != NULL) {
        prometheus_metric_t metric = prometheus_collector_metric_metric(start_time_seconds);
        prometheus_gauge_t guage = prometheus_gauge_cast(metric);
        if (prometheus_gauge_set(guage, process_stat.starttime, NULL) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: %s: set gauge failed", prometheus_metric_name(metric));
        }
        else {
            prometheus_collector_metric_set_state(start_time_seconds, prometheus_metric_collected);
        }
    }
    
    prometheus_process_linux_stat_fini(&process_stat);
}

void prometheus_process_collector_open_fds(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    assert(provider);

    CPE_ERROR(provider->m_em, "xxxxxx: collect open fdns");
    
    prometheus_collector_metric_t open_fds =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_open_fds(provider));
    if (open_fds != NULL) {
        prometheus_metric_t metric = prometheus_collector_metric_metric(open_fds);
        prometheus_gauge_t guage = prometheus_gauge_cast(metric);
        uint32_t count = 0;
        if (prometheus_process_linux_fds_count(provider, &count) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: %s: get count fail", prometheus_metric_name(metric));
        }
        else if (prometheus_gauge_set(guage, count, NULL) != 0) {
            CPE_ERROR(
                provider->m_em, "prometheus: process: collect: %s: set gauge failed", prometheus_metric_name(metric));
        }
        else {
            prometheus_collector_metric_set_state(open_fds, prometheus_metric_collected);
        }
    }
}

void prometheus_process_collector_linux_collect(prometheus_collector_t base_collector) {
    prometheus_process_collector_from_limits(base_collector);
    prometheus_process_collector_from_state(base_collector);
    prometheus_process_collector_open_fds(base_collector);
}
