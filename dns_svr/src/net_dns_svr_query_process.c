#include <assert.h>
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_dns_svr_itf_i.h"
#include "net_dns_svr_query_i.h"
#include "net_dns_svr_query_entry_i.h"

static char const * net_dns_svr_req_print_name(
    net_dns_svr_t svr, write_stream_t ws, char const * p, char const * buf, uint32_t buf_size, uint8_t level);

net_dns_svr_query_t
net_dns_svr_query_parse_request(net_dns_svr_itf_t itf, void const * data, uint32_t data_size) {
    net_dns_svr_t svr = itf->m_svr;

    char const * p = data;

    uint16_t ident;
    CPE_COPY_NTOH16(&ident, p); p+=2;

    uint16_t flags;
    CPE_COPY_NTOH16(&flags, p); p+=2;

    uint8_t qr = flags >> 15;
    if (qr != 0) {
        CPE_ERROR(svr->m_em, "dns-svr: qr=%d, only support query!", qr);
        return NULL;
    }
    
    uint16_t opcode = (flags >> 11) & 15;
    if (opcode != 0) {
        CPE_ERROR(svr->m_em, "dns-svr: opcode %d not support!", opcode);
        return NULL;
    }

    uint16_t qdcount;
    CPE_COPY_NTOH16(&qdcount, p); p+=2;

    p+=2; /* ancount */
    p+=2; /* nscount */
    p+=2; /* arcount */

    net_dns_svr_query_t query = net_dns_svr_query_create(itf, ident);
    if (query == NULL) {
        CPE_ERROR(svr->m_em, "dns-svr: create query fail!");
        return NULL;
    }
    
    mem_buffer_t buffer = net_dns_svr_tmp_buffer(svr);
    
    unsigned int i;
    for(i = 0; i < qdcount; i++) {
        struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
        mem_buffer_clear_data(buffer);
        p = net_dns_svr_req_print_name(svr, (write_stream_t)&ws, p, data, data_size, 0);
        stream_putc((write_stream_t)&ws, 0);

        const char * domain_name = mem_buffer_make_continuous(buffer, 0);

        uint16_t type;
        CPE_COPY_NTOH16(&type, p); p+=2;
        
        uint16_t class;
        CPE_COPY_NTOH16(&class, p); p+=2;

        if (class != 1) {
            CPE_ERROR(svr->m_em, "dns-svr: %s: not support class %d!", domain_name, class);
            continue;
        }
        
        net_address_t address = NULL;
        net_dns_svr_query_entry_type_t entry_type;
        if (type == 1) {
            entry_type = net_dns_svr_query_entry_type_ipv4;
            address = net_address_create_domain(svr->m_schedule, domain_name, 0, NULL);
        }
        else if (type == 28) {
            entry_type = net_dns_svr_query_entry_type_ipv6;
            address = net_address_create_domain(svr->m_schedule, domain_name, 0, NULL);
        }
        else if (type == 12) {
            entry_type = net_dns_svr_query_entry_type_ptr;
            if (!net_schedule_is_domain_address_arpa(svr->m_schedule, domain_name, &address)) {
                CPE_ERROR(svr->m_em, "dns-svr: %s: is not arpa record!", domain_name);
                net_dns_svr_query_free(query);
                return NULL;
            }
        }
        else {
            CPE_ERROR(svr->m_em, "dns-svr: %s: not support query type %d!", domain_name, type);
            continue;
        }

        if (address == NULL) {
            CPE_ERROR(svr->m_em, "dns-svr: %s: create address fail!", domain_name);
            net_dns_svr_query_free(query);
            return NULL;
        }

        net_dns_svr_query_entry_t entry = net_dns_svr_query_entry_create(query, address, entry_type); /*query manage address*/
        if (entry == NULL) {
            CPE_ERROR(svr->m_em, "dns-svr: create query entry fail!");
            net_address_free(address);
            net_dns_svr_query_free(query);
            return NULL;
        }
    }
    
    return query;
}

static uint32_t net_dns_svr_query_calc_entry_query_size(net_dns_svr_query_entry_t query_entry) {
    uint32_t sz = 0;
    
    switch(net_address_type(query_entry->m_address)) {
    case net_address_ipv4:
        sz += 28;
        break;
    case net_address_ipv6:
        sz += 42;
        break;
    case net_address_domain:
        sz += (uint32_t)strlen(net_address_data(query_entry->m_address));
        break;
    }
    
    return sz
        + 2u /*name*/
        + 2u /*type*/
        + 2u /*class*/
        ;
}

static uint32_t net_dns_svr_query_calc_entry_answer_size(net_dns_svr_query_entry_t query_entry) {
    uint8_t i;

    uint32_t sz = 0;
    for(i = 0; i < query_entry->m_result_count; ++i) {
        uint32_t one_sz =
            2 /*name*/
            + 2 /*type*/
            + 2 /*class*/
            + 4 /*ttl*/
            + 2 /*rdlength*/
            ;

        net_address_t address = query_entry->m_results[i];
        switch(net_address_type(address)) {
        case net_address_ipv4:
            one_sz += 4;
            break;
        case net_address_ipv6:
            one_sz += 16;
            break;
        case net_address_domain:
            one_sz += strlen((const char *)net_address_data(address)) + 2;
            break;
        }

        sz +=  one_sz;
    }

    return sz;
}

uint32_t net_dns_svr_query_calc_response_size(net_dns_svr_query_t query) {
    uint32_t sz = 0;

    sz = 2u/*trans_id*/ + 2u/*flag*/ + 2u/*qdcount*/ + 2u/*ancount*/ + 2u/*nscount*/ + 2u/*arcount*/;

    net_dns_svr_query_entry_t entry;
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        sz += net_dns_svr_query_calc_entry_query_size(entry);
        sz += net_dns_svr_query_calc_entry_answer_size(entry);
    }

    return sz;
}

#define net_dns_svr_query_append_name_check_capacity(__sz)              \
    if (*left_capacity < (__sz)) {                                      \
        CPE_ERROR(svr->m_em, "dns-svr: build response: append name: not enouth buf"); \
        return NULL;                                                    \
    }                                                                   \
    else {                                                              \
        *left_capacity -= (__sz);                                       \
    }

static char * net_dns_svr_query_append_name(net_dns_svr_t svr, char * p, void * data, uint32_t * left_capacity, const char * name) {
    net_dns_svr_query_append_name_check_capacity(1);
    char * countp = p++;
    *countp = 0;

    const char * q;
    for(q = name; *q != 0; q++) {
        if (*q != '.') {
            (*countp)++;
            net_dns_svr_query_append_name_check_capacity(1);
            *(p++) = *q;
        }
        else if(*countp != 0) {
            net_dns_svr_query_append_name_check_capacity(1);
            countp = p++;
            *countp = 0;
        }
    }

    if (*countp != 0) {
        net_dns_svr_query_append_name_check_capacity(1);
        *(p++) = 0;
    }

    return p;
}

static char * net_dns_svr_query_append_address(net_dns_svr_t svr, char * p, void * data, uint32_t * left_capacity, net_address_t address) {
    const char * domain_name;
    char buf[128];
    
    switch(net_address_type(address)) {
    case net_address_domain:
        domain_name = (const char *)net_address_data(address);
        break;
    case net_address_ipv4: {
        struct net_address_data_ipv4 * ipv4 = (struct net_address_data_ipv4 * )net_address_data(address);
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d.in-addr.arpa", ipv4->u8[3], ipv4->u8[2], ipv4->u8[1], ipv4->u8[0]);
        domain_name = buf;
        break;
    }
    case net_address_ipv6: {
        const char * maps = "0123456789abcdef";
        struct net_address_data_ipv6 const * addr_data = (struct net_address_data_ipv6 const *)net_address_data(address);
        snprintf(
            buf, sizeof(buf), "%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.%c.ip6.arpa",
            maps[addr_data->u8[15] >> 4], maps[addr_data->u8[15] & 0x0F],
            maps[addr_data->u8[14] >> 4], maps[addr_data->u8[14] & 0x0F],
            maps[addr_data->u8[13] >> 4], maps[addr_data->u8[13] & 0x0F],
            maps[addr_data->u8[12] >> 4], maps[addr_data->u8[12] & 0x0F],
            maps[addr_data->u8[11] >> 4], maps[addr_data->u8[11] & 0x0F],
            maps[addr_data->u8[10] >> 4], maps[addr_data->u8[10] & 0x0F],
            maps[addr_data->u8[9] >> 4], maps[addr_data->u8[9] & 0x0F],
            maps[addr_data->u8[8] >> 4], maps[addr_data->u8[8] & 0x0F],
            maps[addr_data->u8[7] >> 4], maps[addr_data->u8[7] & 0x0F],
            maps[addr_data->u8[6] >> 4], maps[addr_data->u8[6] & 0x0F],
            maps[addr_data->u8[5] >> 4], maps[addr_data->u8[5] & 0x0F],
            maps[addr_data->u8[4] >> 4], maps[addr_data->u8[4] & 0x0F],
            maps[addr_data->u8[3] >> 4], maps[addr_data->u8[3] & 0x0F],
            maps[addr_data->u8[2] >> 4], maps[addr_data->u8[2] & 0x0F],
            maps[addr_data->u8[1] >> 4], maps[addr_data->u8[1] & 0x0F],
            maps[addr_data->u8[0] >> 4], maps[addr_data->u8[0] & 0x0F]);
        domain_name = buf;
        break;
    }
    }

    return net_dns_svr_query_append_name(svr, p, data, left_capacity, domain_name);
}
    
#define net_dns_svr_query_build_response_check_capacity(__sz)           \
    if (left_capacity < (__sz)) {                                       \
        CPE_ERROR(                                                      \
            svr->m_em, "dns-svr: build response: not enouth buf, capacity=%d", \
            capacity);                                                  \
        return -1;                                                      \
    }                                                                   \
    else {                                                              \
        left_capacity -= (__sz);                                        \
    }

int net_dns_svr_query_build_response(net_dns_svr_query_t query, void * data, uint32_t capacity) {
    net_dns_svr_t svr = query->m_itf->m_svr;
    
    net_dns_svr_query_entry_t entry;
    char * p = (char*)data;
    uint32_t left_capacity = capacity;

    net_dns_svr_query_build_response_check_capacity(2);
    CPE_COPY_HTON16(p, &query->m_trans_id); p+=2;

    uint16_t flags = 0;
    flags |= 1u << 15; /*qr*/
    flags |= 1u << 8; /*rd*/
    flags |= 1u << 7; /*ra*/
    net_dns_svr_query_build_response_check_capacity(2);
    CPE_COPY_HTON16(p, &flags); p+=2;

    uint16_t qdcount = 0;
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        qdcount++;
    }
    net_dns_svr_query_build_response_check_capacity(2);
    CPE_COPY_NTOH16(p, &qdcount); p+=2;
    
    uint16_t ancount = 0;
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        ancount += entry->m_result_count;
    }
    net_dns_svr_query_build_response_check_capacity(2);
    CPE_COPY_NTOH16(p, &ancount); p+=2;

    uint16_t nscount = 0;
    net_dns_svr_query_build_response_check_capacity(2);
    CPE_COPY_NTOH16(p, &nscount); p+=2;
    
    uint16_t arcount = 0;
    net_dns_svr_query_build_response_check_capacity(2);
    CPE_COPY_NTOH16(p, &arcount); p+=2;

    /*query*/
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        entry->m_cache_name_offset = (uint16_t)(p - (char*)data);
        p = net_dns_svr_query_append_address(svr, p, data, &left_capacity, entry->m_address);
        if (p == NULL) return -1;
        
        uint16_t qtype = net_dns_svr_query_entry_type_to_qtype(entry->m_type);
        net_dns_svr_query_build_response_check_capacity(2);
        CPE_COPY_HTON16(p, &qtype); p+=2;
    
        uint16_t qclass = 1;
        net_dns_svr_query_build_response_check_capacity(2);
        CPE_COPY_HTON16(p, &qclass); p+=2;
    }

    /*answer*/
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        uint8_t i;
        for(i = 0; i < entry->m_result_count; ++i) {
            uint16_t name_offset = entry->m_cache_name_offset;
            name_offset |= 0xC000u;
            net_dns_svr_query_build_response_check_capacity(2);
            CPE_COPY_HTON16(p, &name_offset); p+=2;

            uint16_t atype = net_dns_svr_query_entry_type_to_qtype(entry->m_type);
            net_dns_svr_query_build_response_check_capacity(2);
            CPE_COPY_HTON16(p, &atype); p+=2;

            uint16_t aclass = 1;
            net_dns_svr_query_build_response_check_capacity(2);
            CPE_COPY_HTON16(p, &aclass); p+=2;

            uint32_t ttl = 300;
            net_dns_svr_query_build_response_check_capacity(4);
            CPE_COPY_HTON32(p, &ttl); p+=4;

            uint16_t rdlength;
            net_address_t address = entry->m_results[i];
            switch(net_address_type(address)) {
            case net_address_ipv4:
                rdlength = 4;
                net_dns_svr_query_build_response_check_capacity(2);
                CPE_COPY_HTON16(p, &rdlength); p+=2;
            
                net_dns_svr_query_build_response_check_capacity(rdlength);
                memcpy(p, net_address_data(address), rdlength);
                p += rdlength;
                break;
            case net_address_ipv6:
                rdlength = 16;
                net_dns_svr_query_build_response_check_capacity(2);
                CPE_COPY_HTON16(p, &rdlength); p+=2;
            
                net_dns_svr_query_build_response_check_capacity(rdlength);
                memcpy(p, net_address_data(address), rdlength);
                p += rdlength;
                break;
            case net_address_domain: {
                net_dns_svr_query_build_response_check_capacity(2);
                char * rdlength_buf = p; p += 2;
                
                p = net_dns_svr_query_append_name(svr, p, data, &left_capacity, (const char *)net_address_data(address));
                if (p == NULL) return -1;

                rdlength = (uint16_t)((p - rdlength_buf) - 2);
                CPE_COPY_HTON16(rdlength_buf, &rdlength);
                break;
            }
            }
        }
    }
    
    return (int)(p - (char*)data);
}

static char const *
net_dns_svr_req_print_name(net_dns_svr_t svr, write_stream_t ws, char const * p, char const * buf, uint32_t buf_size, uint8_t level) {
    while(*p != 0) {
        uint16_t nchars = *(uint8_t const *)p++;
        if((nchars & 0xc0) == 0xc0) {
            uint16_t offset = (nchars & 0x3f) << 8;
            offset |= *(uint8_t const *)p++;

            if (offset > buf_size) {
                CPE_ERROR(svr->m_em, "dns-svr: parse req name: offset %d overflow, buf-size=%d", offset, buf_size);
                return p;
            }

            if (level > 10) {
                CPE_ERROR(svr->m_em, "dns-svr: parse req name: name loop too deep, level=%d", level);
                return p;
            }
            
            net_dns_svr_req_print_name(svr, ws, buf + offset, buf, buf_size, level + 1);

            return p;
        }
        else {
            if (ws) stream_printf(ws, "%*.*s", nchars, nchars, p);
            p += nchars;
        }

        if(*p != 0) {
            if (ws) stream_printf(ws, ".");
        }
    }
    
    p++;
    return p;
}

static void net_dns_svr_req_print(net_dns_svr_t svr, write_stream_t ws, char const * buf, uint32_t buf_size) {
    char const * p = buf;

    uint16_t ident;
    CPE_COPY_NTOH16(&ident, p); p+=2;
    stream_printf(ws, "ident=%#x", ident);

    uint16_t flags;
    CPE_COPY_NTOH16(&flags, p); p+=2;
    stream_printf(ws, ", flags=%#x", flags);
    if (flags >> 15) stream_printf(ws, ", qr");
    stream_printf(ws, ", opcode=%u", (flags >> 11) & 15);
    if ((flags >> 10) & 1) stream_printf(ws, ", aa");
    if ((flags >> 9) & 1) stream_printf(ws, ", tc");
    if ((flags >> 8) & 1) stream_printf(ws, ", rd");
    if ((flags >> 7) & 1) stream_printf(ws, ", ra");
    //stream_printf(ws, ", z=%u", (flags >> 4) & 7);
    stream_printf(ws, ", rcode=%u", flags & 15); 

    uint16_t qdcount;
    CPE_COPY_NTOH16(&qdcount, p); p+=2;
    stream_printf(ws, ", qdcount=%u", qdcount);

    uint16_t ancount;
    CPE_COPY_NTOH16(&ancount, p); p+=2;
    stream_printf(ws, ", ancount=%u", ancount);

    uint16_t nscount;
    CPE_COPY_NTOH16(&nscount, p); p+=2;
    stream_printf(ws, ", nscount=%u", nscount);
 
    uint16_t arcount;
    CPE_COPY_NTOH16(&arcount, p); p+=2;
    stream_printf(ws, ", arcount=%u", arcount);
 
    unsigned int i;
    for(i = 0; i < qdcount; i++) {
        stream_printf(ws, "\n    question[%u]: ", i);
        p = net_dns_svr_req_print_name(svr, ws, p, buf, buf_size, 0);

        uint16_t type;
        CPE_COPY_NTOH16(&type, p); p+=2;
        stream_printf(ws, ", type=%u",type);
        
        uint16_t class;
        CPE_COPY_NTOH16(&class, p); p+=2;
        stream_printf(ws, ", class=%u",class);
    }
 
    for(i = 0; i < ancount; i++) {
        stream_printf(ws, "\n    answer[%u]: ", i);
        p = net_dns_svr_req_print_name(svr, ws, p, buf, buf_size, 0);

        uint16_t type;
        CPE_COPY_NTOH16(&type, p); p+=2;
        stream_printf(ws, ", type=%u", type);

        uint16_t class;
        CPE_COPY_NTOH16(&class, p); p+=2;
        stream_printf(ws, ", class=%u", class);

        uint32_t ttl;
        CPE_COPY_NTOH32(&ttl, p); p+=4;
        stream_printf(ws, ", ttl=%u",ttl);

        uint16_t rdlength;
        CPE_COPY_NTOH16(&rdlength, p); p+=2;
        stream_printf(ws, ", rdlength=%u",rdlength);

        stream_printf(ws, ", rd=");
        uint16_t j;
        for(j = 0; j < rdlength; j++) {
            if (j > 0) stream_printf(ws, "-");
            stream_printf(ws, "%2.2x(%u)", *(uint8_t const *)p, *(uint8_t const *)p);
            p++;
        }
    }
}

const char * net_dns_svr_req_dump(net_dns_svr_t svr, mem_buffer_t buffer, char const * buf, uint32_t buf_size) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    
    net_dns_svr_req_print(svr, (write_stream_t)&stream, buf, buf_size);
    stream_putc((write_stream_t)&stream, 0);

    return mem_buffer_make_continuous(buffer, 0);
}
