#ifndef NET_NDT7_JSON_H_INCLEDED
#define NET_NDT7_JSON_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ndt7_system.h"

NET_BEGIN_DECL

typedef struct yajl_val_s * yajl_val;
typedef struct yajl_gen_t * yajl_gen;

int net_ndt7_response_to_json(yajl_gen gen, net_ndt7_response_t response);
int net_ndt7_response_from_json(net_ndt7_response_t response, yajl_val val, error_monitor_t em);

int net_ndt7_measurement_to_json(yajl_gen gen, net_ndt7_measurement_t measurement);
int net_ndt7_measurement_from_json(net_ndt7_measurement_t measurement, yajl_val val, error_monitor_t em);

NET_END_DECL

#endif
