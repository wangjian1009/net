#ifndef NET_PROTOCOL_I_H_INCLEDED
#define NET_PROTOCOL_I_H_INCLEDED
#include "net_protocol.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_protocol {
    net_schedule_t m_schedule;
    TAILQ_ENTRY(net_protocol) m_next_for_schedule;
    char m_name[32];
    uint8_t m_debug;
    /*protocol*/
    uint16_t m_protocol_capacity;
    net_protocol_init_fun_t m_protocol_init;
    net_protocol_fini_fun_t m_protocol_fini;

    /*endpoint*/
    uint16_t m_endpoint_capacity;
    net_protocol_endpoint_init_fun_t m_endpoint_init;
    net_protocol_endpoint_fini_fun_t m_endpoint_fini;
    net_protocol_endpoint_input_fun_t m_endpoint_input;
    net_protocol_endpoint_on_state_change_fun_t m_endpoint_on_state_chagne;
    net_protocol_endpoint_calc_size_fun_t m_endpoint_calc_size;
    net_protocol_endpoint_dump_fun_t m_endpoint_dump;

    net_endpoint_list_t m_endpoints;
};

NET_END_DECL

#endif
