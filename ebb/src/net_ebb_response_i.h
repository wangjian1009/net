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
            union {
                struct {
                    uint32_t m_tota_size;
                    uint32_t m_writed_size;
                } m_identity;
                struct {
                    uint32_t m_cur_block_total_size;
                    uint32_t m_cur_block_writed_size;
                } m_chunked;
            };
        };
    };
};

void net_ebb_response_real_free(net_ebb_response_t response);

#endif
