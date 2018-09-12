#include "assert.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_strings.h"
#include "net_http_endpoint.h"
#include "net_http_protocol.h"
#include "net_trans_http_endpoint_i.h"
#include "net_trans_http_protocol_i.h"

net_trans_http_endpoint_t
net_trans_http_endpoint_create(net_trans_manage_t manage) {
    net_http_endpoint_t http_endpoint =
        net_http_endpoint_create(
            manage->m_driver, net_endpoint_outbound, net_http_protocol_from_data(manage->m_http_protocol));
    if (http_endpoint == NULL) {
        CPE_ERROR(manage->m_em, "trans: create http-endpoint fail!");
        return NULL;
    }

    net_trans_http_endpoint_t trans_http = net_http_endpoint_data(http_endpoint);
    
    return trans_http;
}

void net_trans_http_endpoint_free(net_trans_http_endpoint_t trans_http) {
    net_http_endpoint_free(net_http_endpoint_from_data(trans_http));
}

int net_trans_http_endpoint_init(net_http_endpoint_t http_endpoint) {
    /* net_trans_http_endpoint_t trahttp = net_endpoint_protocol_data(base_endpoint); */
    /* bzero(&trahttp->m_parser, sizeof(trahttp->m_parser)); */
    return 0;
}

void net_trans_http_endpoint_fini(net_http_endpoint_t http_endpoint) {
    /* net_trans_http_endpoint_t trahttp = net_endpoint_protocol_data(base_endpoint); */
    /* net_trans_ns_parser_fini(&trahttp->m_parser); */
}
