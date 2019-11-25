#ifndef NET_HTTP2_SVR_RESPONSE_H_I_INCLEDED
#define NET_HTTP2_SVR_RESPONSE_H_I_INCLEDED
#include "net_http2_svr_response.h"
#include "net_http2_svr_request_i.h"

struct net_http2_svr_response {
    net_http2_svr_request_t m_request;
    union {
        TAILQ_ENTRY(net_http2_svr_response) m_next;
        struct {
            net_http2_svr_response_state_t m_state;
            net_http2_svr_request_transfer_encoding_t m_transfer_encoding;
            uint16_t m_header_count;
            uint8_t m_head_stream_setted;
            union {
                struct {
                    uint32_t m_tota_size;
                    uint32_t m_writed_size;
                } m_identity;
                struct {
                    uint32_t m_trunk_count;
                } m_chunked;
            };
        };
    };
};

void net_http2_svr_response_real_free(net_http2_svr_response_t response);

#endif
