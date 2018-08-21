#include "assert.h"
#include "cpe/pal/pal_platform.h"
#include "cpe/utils/time_utils.h"
#include "net_address.h"
#include "net_dns_ns_parser.h"
#include "net_dns_ns_pro.h"

static int net_dns_ns_parser_read_head(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size);
static int net_dns_ns_parser_read_qd_record(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size);
static int net_dns_ns_parser_read_an_record(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size);
static int net_dns_ns_parser_read_ns_record(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size);
static int net_dns_ns_parser_read_ar_record(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size);

int net_dns_ns_parser_init(net_dns_ns_parser_t parser, net_dns_manage_t manage, net_dns_source_t source) {
    parser->m_manage = manage;
    parser->m_source = source;
    net_dns_ns_parser_reset(parser);
    return 0;
}

void net_dns_ns_parser_fini(net_dns_ns_parser_t parser) {
}

int net_dns_ns_parser_input(net_dns_ns_parser_t parser, void const * data, uint32_t total_sz) {
    uint8_t const * base = data;

    while(parser->m_r_pos <= total_sz) {
        uint32_t once_size = 0;
        uint8_t const * rp = base + parser->m_r_pos;
        uint32_t left_sz = total_sz - parser->m_r_pos;

        switch(parser->m_state) {
        case net_dns_ns_parser_head:
            if (net_dns_ns_parser_read_head(parser, base, total_sz, rp, left_sz, &once_size) != 0) return -1;
            parser->m_state = net_dns_ns_parser_qd_record;
            break;
        case net_dns_ns_parser_qd_record:
            if (parser->m_record_count >= parser->m_qdcount) {
                parser->m_record_count = 0;
                parser->m_state = net_dns_ns_parser_an_record;
                continue;
            }
            else {
                if (net_dns_ns_parser_read_qd_record(parser, base, total_sz, rp, left_sz, &once_size) != 0) return -1;
                parser->m_record_count++;
            }
            break;
        case net_dns_ns_parser_an_record:
            if (parser->m_record_count >= parser->m_ancount) {
                parser->m_record_count = 0;
                parser->m_state = net_dns_ns_parser_ns_record;
                continue;
            }
            else {
                if (net_dns_ns_parser_read_an_record(parser, base, total_sz, rp, left_sz, &once_size) != 0) return -1;
                parser->m_record_count++;
            }
            break;
        case net_dns_ns_parser_ns_record:
            if (parser->m_record_count >= parser->m_nscount) {
                parser->m_record_count = 0;
                parser->m_state = net_dns_ns_parser_ar_record;
                continue;
            }
            else {
                if (net_dns_ns_parser_read_ns_record(parser, base, total_sz, rp, left_sz, &once_size) != 0) return -1;
                parser->m_record_count++;
            }
            break;
        case net_dns_ns_parser_ar_record:
            if (parser->m_record_count >= parser->m_arcount) {
                parser->m_record_count = 0;
                parser->m_state = net_dns_ns_parser_completed;
                continue;
            }
            else {
                if (net_dns_ns_parser_read_ar_record(parser, base, total_sz, rp, left_sz, &once_size) != 0) return -1;
                parser->m_record_count++;
            }
            break;
        case net_dns_ns_parser_completed:
            break;
        }

        if (once_size == 0) break;

        parser->m_r_pos += once_size;
    }

    return parser->m_state == net_dns_ns_parser_completed ? parser->m_r_pos : 0;
}

void net_dns_ns_parser_reset(net_dns_ns_parser_t parser) {
    parser->m_state = net_dns_ns_parser_head;
    parser->m_r_pos = 0;
    parser->m_record_count = 0;
    parser->m_ident = 0;
    parser->m_flags = 0;
    parser->m_qdcount = 0;
    parser->m_ancount = 0;
    parser->m_nscount = 0;
    parser->m_arcount = 0;
}

static int net_dns_ns_parser_read_head(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size)
{
    if (left_sz < 12) return 0;

    uint8_t const * p = rp;
    
    CPE_COPY_NTOH16(&parser->m_ident, p); p+=2;
    CPE_COPY_NTOH16(&parser->m_flags, p); p+=2;
    CPE_COPY_NTOH16(&parser->m_qdcount, p); p+=2;
    CPE_COPY_NTOH16(&parser->m_ancount, p); p+=2;
    CPE_COPY_NTOH16(&parser->m_nscount, p); p+=2;
    CPE_COPY_NTOH16(&parser->m_arcount, p); p+=2;

    *used_size = (uint32_t)(p - rp);
    
    return 0;
}

static int net_dns_ns_parser_read_qd_record(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size)
{
    uint8_t const * p = net_dns_ns_req_print_name(parser->m_manage, NULL, rp, base, total_sz, 0);
    if (p == NULL) return 0;

    uint32_t name_sz = (uint32_t)(p - rp);
    uint32_t record_sz = name_sz + 2 + 2; /*type + class*/

    if (record_sz > left_sz) return 0;
    
    *used_size = record_sz;
    
    return 0;
}

static int net_dns_ns_parser_read_an_record(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size)
{
    net_dns_manage_t manage = parser->m_manage;

    uint8_t const *  p = rp;

    /*读取名字 */
    p = net_dns_ns_req_dump_name(manage, &parser->m_manage->m_data_buffer, rp, base, total_sz, 0);
    if (p == NULL) return 0;

    const char * hostname = mem_buffer_make_continuous(&parser->m_manage->m_data_buffer, 0);

    uint32_t name_sz = (uint32_t)(p - rp);
    uint32_t record_sz = name_sz + 2 + 2 + 4 + 2; /*type + class + ttl + rdlength*/

    if (record_sz > left_sz) return 0;
    
    uint16_t res_type;
    CPE_COPY_NTOH16(&res_type, p); p+=2;

    uint16_t res_class;
    CPE_COPY_NTOH16(&res_class, p); p+=2;
        
    uint32_t ttl;
    CPE_COPY_NTOH32(&ttl, p); p+=4;

    uint16_t rdlength;
    CPE_COPY_NTOH16(&rdlength, p); p+=2;

    record_sz += rdlength;
    if (record_sz > left_sz) return 0;

    *used_size = record_sz;

    net_address_t address = NULL;
    switch(res_type) {
    case 5: {
        p = net_dns_ns_req_dump_name(manage, net_dns_manage_tmp_buffer(manage), p, base, total_sz, 0);
        assert(p);
        const char * cname = mem_buffer_make_continuous(net_dns_manage_tmp_buffer(manage), 0);

        address = net_address_create_domain(manage->m_schedule, cname, 0, NULL);
        if (address == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: udp <-- create address ipv4 fail");
            return 0;
        }

        break;
    }
    case 1: {
        if (rdlength == 4) {
            struct net_address_data_ipv4 ipv4;
            ipv4.u8[0] = p[0];
            ipv4.u8[1] = p[1];
            ipv4.u8[2] = p[2];
            ipv4.u8[3] = p[3];
            address = net_address_create_from_data_ipv4(manage->m_schedule, &ipv4, 0);
            if (address == NULL) {
                CPE_ERROR(manage->m_em, "dns-cli: udp <-- create address ipv4 fail");
                return 0;
            }
        }
        else {
            if (manage->m_debug) {
                CPE_INFO(manage->m_em, "dns-cli: udp <-- %s answer length %d unknown, ignore record", hostname, rdlength);
            }
            return 0;
        }
        break;
    }
    default:
        if (manage->m_debug) {
            CPE_INFO(manage->m_em, "dns-cli: udp <-- %s type %d not support, ignore record", hostname, res_type);
        }
        return 0;
    }

    assert(address);

    net_dns_manage_add_record( manage, parser->m_source, parser->m_ident, hostname, address, ttl);
    net_address_free(address);
    
    return 0;
}

static int net_dns_ns_parser_read_ns_record(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size)
{
    CPE_ERROR(parser->m_manage->m_em, "dns: ns: parser: not support read ns record!");
    return -1;
}

static int net_dns_ns_parser_read_ar_record(
    net_dns_ns_parser_t parser, uint8_t const * base, uint32_t total_sz,
    uint8_t const * rp, uint32_t left_sz, uint32_t * used_size)
{
    CPE_ERROR(parser->m_manage->m_em, "dns: ns: parser: not support read ar record!");
    return -1;
}
