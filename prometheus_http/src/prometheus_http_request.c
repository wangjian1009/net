#include "prometheus_http_request_i.h"

void prometheus_http_request_on_complete(void * ctx, net_http_svr_request_t request) {
    prometheus_http_processor_t processor = ctx;
    CPE_ERROR(processor->m_em, "xxxx: on request");
}
