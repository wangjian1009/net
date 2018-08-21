#include "cpe/pal/pal_platform.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/stream.h"
#include "net_dns_ns_pro.h"

#define net_dns_ns_req_print_check_size(__sz)   \
    if (((p - buf) + (__sz)) > buf_size) {      \
        stream_printf(ws, "...(%d)", buf_size); \
        return;                                 \
    }                                           \

void net_dns_ns_req_print(net_dns_manage_t manage, write_stream_t ws, uint8_t const * buf, uint32_t buf_size) {
    uint8_t const * p = buf;

    uint16_t ident;
    net_dns_ns_req_print_check_size(2);
    CPE_COPY_NTOH16(&ident, p); p+=2;
    stream_printf(ws, "ident=%#x", ident);

    uint16_t flags;
    net_dns_ns_req_print_check_size(2);
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
    net_dns_ns_req_print_check_size(2);
    CPE_COPY_NTOH16(&qdcount, p); p+=2;
    stream_printf(ws, ", qdcount=%u", qdcount);

    uint16_t ancount;
    net_dns_ns_req_print_check_size(2);
    CPE_COPY_NTOH16(&ancount, p); p+=2;
    stream_printf(ws, ", ancount=%u", ancount);

    uint16_t nscount;
    net_dns_ns_req_print_check_size(2);
    CPE_COPY_NTOH16(&nscount, p); p+=2;
    stream_printf(ws, ", nscount=%u", nscount);
 
    uint16_t arcount;
    net_dns_ns_req_print_check_size(2);
    CPE_COPY_NTOH16(&arcount, p); p+=2;
    stream_printf(ws, ", arcount=%u", arcount);
 
    unsigned int i;
    for(i = 0; i < qdcount; i++) {
        stream_printf(ws, "\n    question[%u]: ", i);
        p = net_dns_ns_req_print_name(manage, ws, p, buf, buf_size, 0);
        if (p == NULL) return;
        
        uint16_t type;
        CPE_COPY_NTOH16(&type, p); p+=2;
        stream_printf(ws, ", type=%u",type);
        
        uint16_t class;
        CPE_COPY_NTOH16(&class, p); p+=2;
        stream_printf(ws, ", class=%u",class);
    }
 
    for(i = 0; i < ancount; i++) {
        stream_printf(ws, "\n    answer[%u]: ", i);
        p = net_dns_ns_req_print_name(manage, ws, p, buf, buf_size, 0);
        if (p == NULL) return;

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

        if (type == 5) {
            stream_printf(ws, ", cname=");
            p = net_dns_ns_req_print_name(manage, ws, p, buf, buf_size, 0);
            if (p == NULL) return;
        }
        else {
            stream_printf(ws, ", rd=");
            uint16_t j;
            for(j = 0; j < rdlength; j++) {
                if (j > 0) stream_printf(ws, "-");
                stream_printf(ws, "%2.2x(%u)", *(uint8_t const *)p, *(uint8_t const *)p);
                p++;
            }
        }
    }
}

const char * net_dns_ns_req_dump(net_dns_manage_t manage, mem_buffer_t buffer, uint8_t const * buf, uint32_t data_size) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    
    net_dns_ns_req_print(manage, (write_stream_t)&stream, buf, data_size);
    stream_putc((write_stream_t)&stream, 0);

    return mem_buffer_make_continuous(buffer, 0);
}

#define net_dns_ns_req_print_name_check_size(__sz)  \
    if (((p - buf) + (__sz)) > buf_size) {          \
        stream_printf(ws, "...(%d)", buf_size);     \
        return NULL;                                \
    }                                               \

uint8_t const * net_dns_ns_req_print_name(
    net_dns_manage_t manage, write_stream_t ws, uint8_t const * p, uint8_t const * buf, uint32_t buf_size, uint8_t level)
{
    net_dns_ns_req_print_name_check_size(2); /*一个ns至少2字节 */
    while(*p != 0) {
        uint16_t nchars = *(uint8_t const *)p++;
        if((nchars & 0xc0) == 0xc0) {
            uint16_t offset = (nchars & 0x3f) << 8;
            offset |= *(uint8_t const *)p++;

            if (offset > buf_size) {
                CPE_ERROR(manage->m_em, "dns-cli: parse req name: offset %d overflow, buf-size=%d", offset, buf_size);
                return p;
            }

            if (level > 10) {
                CPE_ERROR(manage->m_em, "dns-cli: parse req name: name loop too deep, level=%d", level);
                return p;
            }

            net_dns_ns_req_print_name(manage, ws, buf + offset, buf, buf_size, level + 1);

            return p;
        }
        else {
            net_dns_ns_req_print_name_check_size(nchars);
            if (ws) stream_printf(ws, "%*.*s", nchars, nchars, p);
            p += nchars;
        }

        net_dns_ns_req_print_name_check_size(1);
        if(*p != 0) {
            if (ws) stream_printf(ws, ".");
        }

        net_dns_ns_req_print_name_check_size(2); /*一个ns至少2字节 */
    }
    
    p++;
    return p;
}

uint8_t const * net_dns_ns_req_dump_name(
    net_dns_manage_t manage, mem_buffer_t buffer, uint8_t const * p, uint8_t const * buf, uint32_t buf_size, uint8_t level)
{
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    p = net_dns_ns_req_print_name(manage, (write_stream_t)&ws, p, buf, buf_size, 0);

    stream_putc((write_stream_t)&ws, 0);

    return p;
}
