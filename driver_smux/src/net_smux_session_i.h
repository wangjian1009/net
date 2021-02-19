#ifndef NET_SMUX_SESSION_I_H_INCLEDED
#define NET_SMUX_SESSION_I_H_INCLEDED
#include "cpe/utils/hash.h"
#include "net_smux_session.h"
#include "net_smux_protocol_i.h"

NET_BEGIN_DECL

struct net_smux_session {
    net_smux_protocol_t m_protocol;
    TAILQ_ENTRY(net_smux_session) m_next;
    net_smux_config_t m_config;
    uint32_t m_session_id;
    net_smux_runing_mode_t m_runing_mode;

    uint32_t m_max_stream_id; /* next stream identifier */
    struct cpe_hash_table m_streams;

    /*接受缓存剩余空间 */
    uint32_t m_bucket;

    /*等待发送的farme的优先级队列 */
    cpe_priority_queue_t m_shaper;

    struct {
        net_smux_session_underline_type_t m_type;
        union {
            struct {
                net_smux_endpoint_t m_endpoint;
            } m_tcp;
            struct {
                net_smux_dgram_t m_dgram;
                cpe_hash_entry m_hh_for_dgram;
                net_address_t m_remote_address;
            } m_udp;
        };
    } m_underline;

    net_timer_t m_timer_ping;
    net_timer_t m_timer_timeout;
};

net_smux_session_t
net_smux_session_create_dgram(
    net_smux_protocol_t protocol, net_smux_dgram_t dgram, net_address_t remote_address);

void net_smux_session_free(net_smux_session_t session);

uint8_t net_smux_session_debug(net_smux_session_t session);

int net_smux_session_send_frame(
    net_smux_session_t session, net_smux_stream_t stream,
    net_smux_cmd_t cmd, void const * data, uint16_t len, 
    uint64_t expire_ms, uint64_t prio);

int net_smux_session_input(
    net_smux_session_t session, void const * data, uint32_t data_len);

int net_smux_session_dgram_eq(net_smux_session_t l, net_smux_session_t r, void * user_data);
uint32_t net_smux_session_dgram_hash(net_smux_session_t o, void * user_data);

NET_END_DECL

#endif
