#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
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
        p += 2 + 2; /*type + class*/

        net_dns_svr_query_entry_t entry = net_dns_svr_query_entry_create(query, domain_name);
        if (entry == NULL) {
            CPE_ERROR(svr->m_em, "dns-svr: create query entry fail!");
            net_dns_svr_query_free(query);
            return NULL;
        }
    }
    
    return query;
}

static uint32_t net_dns_svr_query_calc_entry_query_size(net_dns_svr_query_entry_t query_entry) {
    return (uint32_t)strlen(query_entry->m_domain_name) + 2u /*name*/
        + 2 /*type*/
        + 2 /*class*/
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
        default:
            continue;
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

static char * net_dns_svr_query_append_name(net_dns_svr_t svr, char * p, void * data, uint32_t capacity, const char * name) {
    char * countp = p++;
    *countp = 0;

    const char * q;
    for(q = name; *q != 0; q++) {
        if(*q != '.') {
            (*countp)++;
            *(p++) = *q;
        }
        else if(*countp != 0) {
            countp = p++;
            *countp = 0;
        }
    }

    if (*countp != 0) {
        *(p++) = 0;
    }

    return p;
}

int net_dns_svr_query_build_response(net_dns_svr_query_t query, void * data, uint32_t capacity) {
    net_dns_svr_t svr = query->m_itf->m_svr;
    
    net_dns_svr_query_entry_t entry;
    char * p = (char*)data;
    
    CPE_COPY_HTON16(p, &query->m_trans_id); p+=2;

    uint16_t flags = 0;
    flags |= 1u << 15; /*qr*/
    
    CPE_COPY_HTON16(p, &flags); p+=2;

    uint16_t qdcount = 0;
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        qdcount++;
    }
    CPE_COPY_NTOH16(p, &qdcount); p+=2;
    
    uint16_t ancount = 0;
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        uint8_t i;
        for(i = 0; i < entry->m_result_count; ++i) {
            net_address_t address = entry->m_results[i];
            switch(net_address_type(address)) {
            case net_address_ipv4:
            case net_address_ipv6:
                ancount++;
                break;
            default:
                continue;
            }
        }
    }
    CPE_COPY_NTOH16(p, &ancount); p+=2;

    uint16_t nscount = 0;
    CPE_COPY_NTOH16(p, &nscount); p+=2;
    
    uint16_t arcount = 0;
    CPE_COPY_NTOH16(p, &arcount); p+=2;

    /*query*/
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        entry->m_cache_name_offset = (uint16_t)(p - (char*)data);
        p = net_dns_svr_query_append_name(svr, p, data, capacity, entry->m_domain_name);

        uint16_t qtype = 1;
        CPE_COPY_HTON16(p, &qtype); p+=2;
    
        uint16_t qclass = 1;
        CPE_COPY_HTON16(p, &qclass); p+=2;
    }

    /*answer*/
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        uint8_t i;
        for(i = 0; i < entry->m_result_count; ++i) {
            uint16_t rdlength;
            
            net_address_t address = entry->m_results[i];
            switch(net_address_type(address)) {
            case net_address_ipv4:
                rdlength = 4;
                break;
            case net_address_ipv6:
                rdlength = 16;
                break;
            default:
                continue;
            }

            uint16_t name_offset = entry->m_cache_name_offset;
            name_offset |= 0xC000u;
            CPE_COPY_HTON16(p, &name_offset); p+=2;

            uint16_t atype = 1;
            CPE_COPY_HTON16(p, &atype); p+=2;
    
            uint16_t aclass = 1;
            CPE_COPY_HTON16(p, &aclass); p+=2;

            uint32_t ttl = 300;
            CPE_COPY_HTON32(p, &ttl); p+=4;

            CPE_COPY_HTON16(p, &rdlength); p+=2;
            
            memcpy(p, net_address_data(address), rdlength);
            p += rdlength;
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
    stream_printf(ws, ", qr=%u", flags >> 15);
    stream_printf(ws, ", opcode=%u", (flags >> 11) & 15);
    stream_printf(ws, ", aa=%u", (flags >> 10) & 1);
    stream_printf(ws, ", tc=%u", (flags >> 9) & 1);
    stream_printf(ws, ", rd=%u", (flags >> 8) & 1);
    stream_printf(ws, ", ra=%u", (flags >> 7) & 1);
    stream_printf(ws, ", z=%u", (flags >> 4) & 7);
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
