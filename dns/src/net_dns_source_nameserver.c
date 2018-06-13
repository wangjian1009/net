#include "cpe/pal/pal_platform.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/stream.h"
#include "cpe/utils/buffer.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_timer.h"
#include "net_dns_dgram_receiver.h"
#include "net_dns_task.h"
#include "net_dns_task_ctx.h"
#include "net_dns_source_nameserver_i.h"

static int net_dns_source_nameserver_init(net_dns_source_t source);
static void net_dns_source_nameserver_fini(net_dns_source_t source);
static void net_dns_source_nameserver_dump(write_stream_t ws, net_dns_source_t source);
static void net_dns_source_nameserver_dgram_receiver(void * ctx, void * data, size_t data_size);

static int net_dns_source_nameserver_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static void net_dns_source_nameserver_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static int net_dns_source_nameserver_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static void net_dns_source_nameserver_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

static const char * net_dns_source_nameserver_req_dump(mem_buffer_t buffer, char const * buf);

net_dns_source_nameserver_t
net_dns_source_nameserver_create(net_dns_manage_t manage, net_address_t addr, uint8_t is_own) {
    net_dns_source_t source =
        net_dns_source_create(
            manage,
            sizeof(struct net_dns_source_nameserver),
            net_dns_source_nameserver_init,
            net_dns_source_nameserver_fini,
            net_dns_source_nameserver_dump,
            0,
            net_dns_source_nameserver_ctx_init,
            net_dns_source_nameserver_ctx_fini,
            net_dns_source_nameserver_ctx_start,
            net_dns_source_nameserver_ctx_cancel);
    if (source == NULL) return NULL;

    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);

    if (is_own) {
        nameserver->m_address = addr;
    }
    else {
        nameserver->m_address = net_address_copy(manage->m_schedule, addr);
        if (nameserver->m_address == NULL) {
            CPE_ERROR(manage->m_em, "dns: nameserver: copy address fail!");
            net_dns_source_free(source);
            return NULL;
        }
    }

    if (net_address_port(nameserver->m_address) == 0) {
        net_address_set_port(nameserver->m_address, 53);
    }

    if (net_dns_dgram_receiver_create(
            manage, 0, nameserver->m_address, nameserver, net_dns_source_nameserver_dgram_receiver)
        == NULL)
    {
        CPE_ERROR(manage->m_em, "dns: nameserver: create dgram receiver fail!");
        if (!is_own) net_address_free(nameserver->m_address);
        net_dns_source_free(source);
        return NULL;
    }

    if (manage->m_debug >= 2) {
        CPE_INFO(
            manage->m_em, "dns: %s created!",
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), source));
    }
    
    return nameserver;
}

void net_dns_source_nameserver_free(net_dns_source_nameserver_t nameserver) {
    net_dns_source_free(net_dns_source_from_data(nameserver));
}

int net_dns_source_nameserver_init(net_dns_source_t source) {
    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);

    nameserver->m_address = NULL;
    nameserver->m_endpoint = NULL;
    
    return 0;
}

void net_dns_source_nameserver_fini(net_dns_source_t source) {
    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);

    net_dns_dgram_receiver_t receiver =
        net_dns_dgram_receiver_find_by_ctx(
            net_dns_source_manager(source), nameserver, net_dns_source_nameserver_dgram_receiver);
    if (receiver) {
        net_dns_dgram_receiver_free(receiver);
    }

    if (nameserver->m_address) {
        net_address_free(nameserver->m_address);
        nameserver->m_address = NULL;
    }

    if (nameserver->m_endpoint) {
        net_endpoint_free(nameserver->m_endpoint);
        nameserver->m_endpoint = NULL;
    }
}

void net_dns_source_nameserver_dump(write_stream_t ws, net_dns_source_t source) {
    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);
    
    stream_printf(ws, "nameserver[");
    if (nameserver->m_address) {
        net_address_print(ws, nameserver->m_address);
    }
    stream_printf(ws, "]");
}

int net_dns_source_nameserver_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    return 0;
}

void net_dns_source_nameserver_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}

int net_dns_source_nameserver_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    net_dns_task_t task = net_dns_task_ctx_task(task_ctx);
    net_dns_manage_t manage = net_dns_task_ctx_manage(task_ctx);
    net_dns_source_nameserver_t nameserver = net_dns_source_data(source);

    mem_buffer_t buffer = &manage->m_data_buffer;
    mem_buffer_clear_data(buffer);

    char * buf = mem_buffer_alloc(buffer, 512);
    if (buf == NULL) {
        CPE_ERROR(manage->m_em, "dns: build nameserver req: alloc fail!");
        return -1; 
    }

    char *p = buf;

    /* Transaction ID */
    uint16_t task_id = net_dns_task_id(task);
    CPE_COPY_HTON16(p, &task_id); p+=2;

    uint16_t flag = 0x0100;
    CPE_COPY_HTON16(p, &flag); p+=2;

    uint16_t qdcount = 1;
    CPE_COPY_HTON16(p, &qdcount); p+=2;

    uint16_t ancount = 0;
    CPE_COPY_HTON16(p, &ancount); p+=2;

    uint16_t nscount = 0;
    CPE_COPY_HTON16(p, &nscount); p+=2;
 
    uint16_t arcount = 0;
    CPE_COPY_HTON16(p, &arcount); p+=2;
    
    /* Query section */
    char * countp = p;  
    *(p++) = 0;

    const char * q;
    for(q = net_dns_task_hostname(task); *q != 0; q++) {
        if(*q != '.') {
            (*countp)++;
            *(p++) = *q;
        }
        else if(*countp != 0) {
            countp = p;
            *(p++) = 0;
        }
    }

    if (*countp != 0) {
        *(p++) = 0;
    }
 
    //Type=1(A):host address
    uint16_t qtype = 1;
    CPE_COPY_HTON16(p, &qtype); p+=2;
    
    //Class=1(IN):internet
    uint16_t qclass = 1;
    CPE_COPY_HTON16(p, &qclass); p+=2;

    if (net_dns_manage_dgram_send(manage, nameserver->m_address, buf, p - buf) < 0) {
        CPE_ERROR(manage->m_em, "dns: nameserver: send data fail");
        return -1;
    }

    if (manage->m_debug >= 2) {
        CPE_INFO(
            manage->m_em, "dns: udp --> %s",
            net_dns_source_nameserver_req_dump(net_dns_manage_tmp_buffer(manage), buf));
    }
    
    return 0;
}

void net_dns_source_nameserver_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}

static void net_dns_source_nameserver_dgram_receiver(void * ctx, void * data, size_t data_size) {
    net_dns_source_nameserver_t nameserver = ctx;
    net_dns_source_t source = net_dns_source_from_data(nameserver);
    net_dns_manage_t manage = net_dns_source_manager(source);

    if (manage->m_debug >= 2) {
        CPE_INFO(
            manage->m_em, "dns: udp <-- %s",
            net_dns_source_nameserver_req_dump(net_dns_manage_tmp_buffer(manage), data));
    }
}

static char const *
net_dns_source_nameserver_req_print_name(write_stream_t ws, char const * p, char const * buf) {
    uint16_t nchars = (uint8_t)*(p++);
    if((nchars & 0xc0) == 0xc0) {
        uint16_t offset = (nchars & 0x3f) << 8;
        offset |= (uint16_t)*(p++);
        nchars = (uint16_t)buf[offset++];
        stream_printf(ws, "%*.*bs", nchars, nchars, buf + offset);
    }
    else {
        stream_printf(ws, "%*.*s", nchars, nchars, p);
        p += nchars;
    }
 
    return p;
}

static void net_dns_source_nameserver_req_print(write_stream_t ws, char const * buf) {
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
        stream_printf(ws, "\n    qd[%u]: ", i);
        while(*p!=0) {
            p = net_dns_source_nameserver_req_print_name(ws, p, buf);
            if(*p != 0) {
                stream_printf(ws, ".");
            }
        }
        p++;

        uint16_t type;
        CPE_COPY_NTOH16(&type, p); p+=2;
        stream_printf(ws, ", type=%u",type);
        
        uint16_t class;
        CPE_COPY_NTOH16(&class, p); p+=2;
        stream_printf(ws, ", class=%u",class);
    }
 
    for(i = 0; i < ancount; i++) {
        stream_printf(ws, "\n    an[%u]: ", i);
        p = net_dns_source_nameserver_req_print_name(ws, p, buf);

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

static const char * net_dns_source_nameserver_req_dump(mem_buffer_t buffer, char const * buf) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    
    net_dns_source_nameserver_req_print((write_stream_t)&stream, buf);
    stream_putc((write_stream_t)&stream, 0);

    return mem_buffer_make_continuous(buffer, 0);
}
