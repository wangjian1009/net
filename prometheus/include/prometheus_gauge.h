#ifndef PROMETHEUS_GAUGE_H_INCLEDED
#define PROMETHEUS_GAUGE_H_INCLEDED
#include "prometheus_types.h"

CPE_BEGIN_DECL

prometheus_gauge_t
prometheus_gauge_create(
    prometheus_manager_t manager,
    const char *name, const char *help,
    size_t label_key_count, const char **label_keys);

void prometheus_gauge_free(prometheus_gauge_t gauge);

CPE_END_DECL

#endif
