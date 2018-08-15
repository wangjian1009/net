#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/buffer.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_proxy_http_svr_endpoint_i.h"
#include "net_proxy_http_svr_i.h"
#include "net_proxy_http_pro.h"

static int net_proxy_http_svr_send_proxy_http_response(net_endpoint_t endpoint, net_address_t address);
    
int net_proxy_http_svr_endpoint_init(net_endpoint_t endpoint) {
    net_proxy_http_svr_endpoint_t proxy_http_svr = net_endpoint_protocol_data(endpoint);
    proxy_http_svr->m_stage = net_proxy_http_svr_endpoint_stage_init;
    proxy_http_svr->m_on_connect_fun = NULL;
    proxy_http_svr->m_on_connect_ctx = NULL;
    return 0;
}

void net_proxy_http_svr_endpoint_fini(net_endpoint_t endpoint) {
}

void net_proxy_http_svr_endpoint_set_connect_fun(
    net_proxy_http_svr_endpoint_t ss_ep,
    net_proxy_http_svr_connect_fun_t on_connect_fon, void * on_connect_ctx)
{
    ss_ep->m_on_connect_fun = on_connect_fon;
    ss_ep->m_on_connect_ctx = on_connect_ctx;
}

int net_proxy_http_svr_endpoint_input(net_endpoint_t endpoint) {
    //net_schedule_t schedule = net_endpoint_schedule(endpoint);
    /* error_monitor_t em = net_schedule_em(schedule); */
    /* net_proxy_http_svr_t proxy_http_svr = net_protocol_data(net_endpoint_protocol(endpoint)); */
    /* net_proxy_http_svr_endpoint_t ss_ep = net_endpoint_protocol_data(endpoint); */
    /* void * data; */

    return 0;
}

int net_proxy_http_svr_endpoint_forward(net_endpoint_t endpoint, net_endpoint_t from) {
    return net_endpoint_wbuf_append_from_other(endpoint, from, 0);
}
