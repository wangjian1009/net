#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_proxy_http_svr_endpoint_i.h"

const char * proxy_http_svr_basic_read_state_str(proxy_http_svr_basic_read_state_t state);
const char * proxy_http_svr_basic_write_state_str(proxy_http_svr_basic_write_state_t state);

static void net_proxy_http_svr_endpoint_basic_set_read_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_read_state_t read_state);

static int net_proxy_http_svr_endpoint_basic_first_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    char * line);

int net_proxy_http_svr_endpoint_basic_read_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data)
{
    char * line = data;
    char * sep = strstr(line, "\r\n");
    for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        if (net_proxy_http_svr_endpoint_basic_first_header_line(http_protocol, http_ep, endpoint, line) != 0) return -1;
        *sep = '\r';
    }

    if (line[0]) {
        if (net_proxy_http_svr_endpoint_basic_first_header_line(http_protocol, http_ep, endpoint, line) != 0) return -1;
    }

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_first_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    char * line)
{
    return 0;
}

static void net_proxy_http_svr_endpoint_basic_set_read_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_read_state_t read_state)
{
    if (http_ep->m_basic.m_read_state == read_state) return;

    if (http_ep->m_debug) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: read-state %s ==> %s",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            proxy_http_svr_basic_read_state_str(http_ep->m_basic.m_read_state),
            proxy_http_svr_basic_read_state_str(read_state));
    }

    http_ep->m_basic.m_read_state = read_state;
}

const char * proxy_http_svr_basic_read_state_str(proxy_http_svr_basic_read_state_t state) {
    switch(state) {
    case proxy_http_svr_basic_read_state_reading_header:
        return "r-reading-header";
    case proxy_http_svr_basic_read_state_reading_content:
        return "r-reading-content";
    }
}

const char * proxy_http_svr_basic_write_state_str(proxy_http_svr_basic_write_state_t state) {
    switch(state) {
    case proxy_http_svr_basic_write_state_invalid:
        return "w-invalid";
    case proxy_http_svr_basic_write_state_sending_content_response:
        return "w-sending-content-response";
    case proxy_http_svr_basic_write_state_forwarding:
        return "w-forwarding";
    case proxy_http_svr_basic_write_state_stopped:
        return "w-stoped";
    }
}
