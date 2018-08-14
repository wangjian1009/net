#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "net_dns_svr_itf_i.h"
#include "net_dns_svr_query_i.h"
#include "net_dns_svr_query_entry_i.h"

static char const * net_dns_svr_req_print_name(write_stream_t ws, char const * p, char const * buf, uint8_t level);

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
        p = net_dns_svr_req_print_name((write_stream_t)&ws, p, data, 0);
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

static uint32_t net_dns_svr_query_calc_entry_answer_size(net_dns_svr_query_entry_t query_entry) {
    if (query_entry->m_result == NULL) return 0;
    
    return (uint32_t)strlen(query_entry->m_domain_name) + 1u /*name*/
        + 2u/*type*/
        + 2u/*class*/
        + 4u/*ttl*/
        + 2u/*data-length*/
        ;
}

uint32_t net_dns_svr_query_calc_response_size(net_dns_svr_query_t query) {
    uint32_t sz = 0;

    sz = 2u/*trans_id*/ + 2u/*flag*/ + 2u/*qdcount*/ + 2u/*ancount*/ + 2u/*nscount*/ + 2u/*arcount*/;

    net_dns_svr_query_entry_t entry;
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        sz += net_dns_svr_query_calc_entry_answer_size(entry);
    }

    return sz;
}

int net_dns_svr_query_build_response(net_dns_svr_query_t query, void * data, uint32_t capacity) {
    net_dns_svr_query_entry_t entry;
    char * p = (char*)data;
    
    CPE_COPY_HTON16(p, &query->m_trans_id); p+=2;

    char flags[2];
    bzero(flags, sizeof(flags));

    uint16_t qdcount = 0;
    CPE_COPY_NTOH16(p, &qdcount); p+=2;
    
    uint16_t ancount = 0;
    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        if (entry->m_result) ancount++;
    }
    CPE_COPY_NTOH16(p, &ancount); p+=2;

    uint16_t nscount = 0;
    CPE_COPY_NTOH16(p, &nscount); p+=2;
    
    uint16_t arcount = 0;
    CPE_COPY_NTOH16(p, &arcount); p+=2;

    TAILQ_FOREACH(entry, &query->m_entries, m_next) {
        if (entry->m_result == NULL) continue;
    }
    
    return (int)(p - (char*)data);
}

static char const *
net_dns_svr_req_print_name(write_stream_t ws, char const * p, char const * buf, uint8_t level) {
    while(*p != 0) {
        uint16_t nchars = *(uint8_t const *)p++;
        if((nchars & 0xc0) == 0xc0) {
            uint16_t offset = (nchars & 0x3f) << 8;
            offset |= *(uint8_t const *)p++;

            if (level != 0) return p;
            
            const char * p2 = buf + offset;
            int protect;
            for(protect = 0; protect < 100 && *p2 != 0; protect++) {
                p2 = net_dns_svr_req_print_name(ws, p2, buf, level + 1);
                if(*p2 != 0) {
                    if (ws) stream_printf(ws, ".");
                }
            }

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

static void net_dns_svr_req_print(write_stream_t ws, char const * buf) {
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
        p = net_dns_svr_req_print_name(ws, p, buf, 0);

        uint16_t type;
        CPE_COPY_NTOH16(&type, p); p+=2;
        stream_printf(ws, ", type=%u",type);
        
        uint16_t class;
        CPE_COPY_NTOH16(&class, p); p+=2;
        stream_printf(ws, ", class=%u",class);
    }
 
    for(i = 0; i < ancount; i++) {
        stream_printf(ws, "\n    answer[%u]: ", i);
        p = net_dns_svr_req_print_name(ws, p, buf, 0);

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

const char * net_dns_svr_req_dump(mem_buffer_t buffer, char const * buf) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    
    net_dns_svr_req_print((write_stream_t)&stream, buf);
    stream_putc((write_stream_t)&stream, 0);

    return mem_buffer_make_continuous(buffer, 0);
}
