#ifndef PROMETHEUS_COLLECTOR_H_INCLEDED
#define PROMETHEUS_COLLECTOR_H_INCLEDED
#include "prometheus_types.h"

NET_BEGIN_DECL

prometheus_collector_t
prometheus_collector_create(prometheus_collector_t manager, const char * name);
void prometheus_collector_free(prometheus_collector_t collector);

NET_END_DECL

#endif
