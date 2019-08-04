#ifndef NET_EBB_RESPONSE_H_I_INCLEDED
#define NET_EBB_RESPONSE_H_I_INCLEDED
#include "net_ebb_response.h"
#include "net_ebb_request_i.h"

struct net_ebb_response {
    net_ebb_request_t m_request;
    union {
        TAILQ_ENTRY(net_ebb_response) m_next;
        struct {
            net_ebb_response_state_t m_state;
            net_ebb_request_transfer_encoding_t m_transfer_encoding;
            uint16_t m_header_count;
            uint8_t m_head_connection_setted;
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

void net_ebb_response_real_free(net_ebb_response_t response);

#endif
