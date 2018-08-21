#ifndef NET_DNS_NS_PARSER_H_INCLEDED
#define NET_DNS_NS_PARSER_H_INCLEDED
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

typedef enum net_dns_ns_parser_state {
    net_dns_ns_parser_head,
    net_dns_ns_parser_qd_record,
    net_dns_ns_parser_an_record,
    net_dns_ns_parser_ns_record,
    net_dns_ns_parser_ar_record,
    net_dns_ns_parser_completed,
} net_dns_ns_parser_state_t;

struct net_dns_ns_parser {
    net_dns_manage_t m_manage;
    net_dns_source_t m_source;
    net_dns_ns_parser_state_t m_state;
    uint32_t m_r_pos;
    uint16_t m_record_count;
    uint16_t m_ident;
    uint16_t m_flags;
    uint16_t m_qdcount;
    uint16_t m_ancount;
    uint16_t m_nscount;
    uint16_t m_arcount;
};

int net_dns_ns_parser_init(net_dns_ns_parser_t parser, net_dns_manage_t manage, net_dns_source_t source);
void net_dns_ns_parser_fini(net_dns_ns_parser_t parser);

void net_dns_ns_parser_reset(net_dns_ns_parser_t parser);
int net_dns_ns_parser_input(net_dns_ns_parser_t parser, void const * data, uint32_t data_size);

NET_END_DECL

#endif
