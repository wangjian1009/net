#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream.h"
#include "prometheus_manager_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_sample_i.h"
#include "prometheus_collector_metric_i.h"
#include "prometheus_collector_i.h"

void prometheus_manager_collect_metric(write_stream_t ws, prometheus_metric_t metric) {
    prometheus_metric_category_t category = prometheus_metric_category(metric);
        
    stream_printf(ws, "# HELP %s %s\n", metric->m_name, metric->m_help);
    stream_printf(ws, "# TYPE %s %s\n", metric->m_name, prometheus_metric_category_str(category));

    struct cpe_hash_it sample_it;
    cpe_hash_it_init(&sample_it, &metric->m_samples);

    prometheus_metric_sample_t sample;
    while((sample = cpe_hash_it_next(&sample_it))) {
        if (category == prometheus_metric_histogram) {
    /*         prometheus_metric_sample_histogram_t * hist_sample = (prometheus_metric_sample_histogram_t *)prometheus_map_get(metric->samples, key); */

    /*         if (hist_sample == NULL) return 1; */

    /*         for (prometheus_linked_list_node_t * current_hist_node = hist_sample->l_value_list->head; current_hist_node != NULL; */
    /*              current_hist_node = current_hist_node->next) { */
    /*             const char * hist_key = (const char *)current_hist_node->item; */
    /*             prometheus_metric_sample_t * sample = (prometheus_metric_sample_t *)prometheus_map_get(hist_sample->samples, hist_key); */
    /*             if (sample == NULL) return 1; */
    /*             r = prometheus_metric_formatter_load_sample(formatter, sample); */
    /*             if (r) return r; */
    /*         } */
        } else {
            stream_printf(ws, "%s %.17g\n", sample->m_l_value, sample->m_r_value);
        }
    }

    stream_printf(ws, "\n");
}

void prometheus_manager_collect(write_stream_t ws, prometheus_manager_t manager) {
    prometheus_collector_t collector;

    TAILQ_FOREACH(collector, &manager->m_collectors, m_next) {
        /*   prom_map_t *metrics = collector->collect_fn(collector); */
        /*   if (metrics == NULL) return 1; */

        prometheus_collector_metric_t collector_metric;
        TAILQ_FOREACH(collector_metric, &collector->m_metrics, m_next_for_collector) {
            prometheus_metric_t metric = collector_metric->m_metric;
            prometheus_manager_collect_metric(ws, metric);
        }
    }
}
