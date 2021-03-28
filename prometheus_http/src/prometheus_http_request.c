#include "cpe/pal/pal_string.h"
#include "net_http_svr_request.h"
#include "net_http_svr_response.h"
#include "prometheus_manager.h"
#include "prometheus_http_request_i.h"

void prometheus_http_request_on_complete(void * ctx, net_http_svr_request_t request) {
    prometheus_http_processor_t processor = ctx;

    net_http_svr_response_t response = net_http_svr_response_create(request);

    if (net_http_svr_request_method(request) != net_http_svr_request_method_get) {
        net_http_svr_response_append_code(response, 400, "Invalid HTTP Method");
        goto COMPLETE;
    }

    const char * path = net_http_svr_request_relative_path(request);
    
    if (strcmp(path, "") != 0) {
        net_http_svr_response_append_code(response, 400, "Bad Request");
        goto COMPLETE;
    }

    const char * buf = prometheus_manager_collect_dump(&processor->m_collect_buffer, processor->m_manager);
    net_http_svr_response_append_code(response, 200, "OK");

    net_http_svr_response_append_body_identity_begin(response, strlen(buf));
    net_http_svr_response_append_body_identity_data(response, buf, strlen(buf));
    
COMPLETE:
    net_http_svr_response_append_complete(response);
}

/* int promhttp_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method, */
/*                      const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) { */
/* } */

