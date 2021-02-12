#ifndef NET_HTTP2_STREAM_ENDPOINT_I_H_INCLEDED
#define NET_HTTP2_STREAM_ENDPOINT_I_H_INCLEDED
#include "net_http2_stream_endpoint.h"
#include "net_http2_stream_driver_i.h"

struct net_http2_stream_endpoint {
    net_endpoint_t m_base_endpoint;
    net_http2_endpoint_t m_control;
    TAILQ_ENTRY(net_http2_stream_endpoint) m_next_for_control;
    uint8_t m_send_scheduled;
    uint8_t m_send_processing;
    TAILQ_ENTRY(net_http2_stream_endpoint) m_next_for_sending;
    int32_t m_stream_id;
    char * m_path;
};

int net_http2_stream_endpoint_init(net_endpoint_t base_endpoint);
void net_http2_stream_endpoint_fini(net_endpoint_t base_endpoint);
int net_http2_stream_endpoint_connect(net_endpoint_t base_endpoint);
int net_http2_stream_endpoint_update(net_endpoint_t base_endpoint);
int net_http2_stream_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay);
int net_http2_stream_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss);

/**/
int net_http2_stream_endpoint_set_state(net_http2_stream_endpoint_t stream, net_http2_stream_endpoint_state_t state);

/*http2*/
int net_http2_stream_endpoint_sync_state(net_http2_stream_endpoint_t stream);

int net_http2_stream_endpoint_delay_send_data(net_http2_stream_endpoint_t stream);
void net_http2_stream_endpoint_schedule_send_data(net_http2_stream_endpoint_t stream);
void net_http2_stream_endpoint_schedule_send_data(net_http2_stream_endpoint_t stream);

#endif
