#ifndef NET_HTTP_ENDPOINT_I_H_INCLEDED
#define NET_HTTP_ENDPOINT_I_H_INCLEDED
#include "net_http_endpoint.h"
#include "net_http_protocol_i.h"

#define net_ep_buf_http_in net_ep_buf_user1
#define net_ep_buf_http_out net_ep_buf_user2
#define net_ep_buf_http_body net_ep_buf_user3

NET_BEGIN_DECL

typedef enum net_http_res_state {
    net_http_res_state_reading_head_first,
    net_http_res_state_reading_head_follow,
    net_http_res_state_reading_body,
    net_http_res_state_completed,
} net_http_res_state_t;

struct net_http_endpoint {
    net_endpoint_t m_endpoint;
    net_http_connection_type_t m_connection_type;

    /*runtime*/
    int64_t m_connecting_time_ms;
    net_timer_t m_process_timer;

    uint16_t m_req_count;
    net_http_req_list_t m_reqs;

    struct net_http_res_ctx {
        net_http_req_t m_req;
        net_http_res_state_t m_state;
        net_http_transfer_encoding_t m_trans_encoding;
        struct {
            uint32_t m_length;
        } m_res_content;
        struct {
            net_http_trunked_state_t m_state;
            uint16_t m_count;
            uint32_t m_length;
        } m_res_trunked;
    }  m_current_res;
};

int net_http_endpoint_init(net_endpoint_t endpoint);
void net_http_endpoint_fini(net_endpoint_t endpoint);
int net_http_endpoint_input(net_endpoint_t endpoint);
int net_http_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state);

int net_http_endpoint_write(
    net_http_protocol_t http_protocol,
    net_http_endpoint_t http_ep, net_http_req_t http_req, void const * data, uint32_t size);

int net_http_endpoint_write_head_pair(
    net_http_endpoint_t http_ep, net_http_req_t http_req, const char * attr_name, const char * attr_value);

int net_http_endpoint_do_process(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_endpoint_t endpoint);

const char * net_http_res_state_str(net_http_res_state_t res_state);

NET_END_DECL

#endif
