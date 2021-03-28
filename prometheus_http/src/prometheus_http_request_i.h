#ifndef PROMETHEUS_HTTP_REQUEST_I_H_INCLEDED
#define PROMETHEUS_HTTP_REQUEST_I_H_INCLEDED
#include "net_http_svr_request.h"
#include "prometheus_http_processor_i.h"

void prometheus_http_request_on_complete(void * ctx, net_http_svr_request_t request);

#endif
