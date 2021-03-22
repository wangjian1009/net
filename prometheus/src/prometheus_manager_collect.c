#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream.h"
#include "prometheus_manager_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_sample_i.h"
#include "prometheus_metric_sample_histogram_i.h"
#include "prometheus_collector_metric_i.h"
#include "prometheus_collector_i.h"

void prometheus_manager_collect_metric(write_stream_t ws, prometheus_metric_t metric) {
    prometheus_metric_category_t category = prometheus_metric_category(metric);
        
    stream_printf(ws, "# HELP %s %s\n", metric->m_name, metric->m_help);
    stream_printf(ws, "# TYPE %s %s\n", metric->m_name, prometheus_metric_category_str(category));

    /*sample-value*/
    struct cpe_hash_it sample_it;
    cpe_hash_it_init(&sample_it, &metric->m_samples);

    prometheus_metric_sample_t sample;
    while((sample = cpe_hash_it_next(&sample_it))) {
        stream_printf(ws, "%s %.17g\n", sample->m_l_value, sample->m_r_value);
    }

    /*sample-histogram*/
    struct cpe_hash_it histogram_it;
    cpe_hash_it_init(&histogram_it, &metric->m_sample_histograms);

    prometheus_metric_sample_histogram_t histogram;
    while((histogram = cpe_hash_it_next(&histogram_it))) {
        TAILQ_FOREACH(sample, &histogram->m_samples, m_owner_histogram.m_next) {
            stream_printf(ws, "%s %.17g\n", sample->m_l_value, sample->m_r_value);
        }
    }
}

void prometheus_manager_collect(write_stream_t ws, prometheus_manager_t manager) {
    prometheus_collector_t collector;

    TAILQ_FOREACH(collector, &manager->m_collectors, m_next) {
        prometheus_collector_metric_t collector_metric;

        if (collector->m_collect) {
            TAILQ_FOREACH(collector_metric, &collector->m_metrics, m_next_for_collector) {
                collector_metric->m_state = prometheus_metric_not_collect;
            }
            collector->m_collect(collector);
        }
        else {
            TAILQ_FOREACH(collector_metric, &collector->m_metrics, m_next_for_collector) {
                collector_metric->m_state = prometheus_metric_collected;
            }
        }
        
        TAILQ_FOREACH(collector_metric, &collector->m_metrics, m_next_for_collector) {
            if (collector_metric->m_state == prometheus_metric_collected) {
                prometheus_manager_collect_metric(ws, collector_metric->m_metric);
            }
        }
    }
}
