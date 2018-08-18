#include "cpe/pal/pal_platform.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/stream.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/time_utils.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_dgram.h"
#include "net_timer.h"
#include "net_dns_task.h"
#include "net_dns_task_step.h"
#include "net_dns_task_ctx.h"
#include "net_dns_entry.h"
#include "net_dns_entry_item.h"
#include "net_dns_source_ns_i.h"
#include "net_dns_source_i.h"

static int net_dns_source_ns_init(net_dns_source_t source);
static void net_dns_source_ns_fini(net_dns_source_t source);
static void net_dns_source_ns_dump(write_stream_t ws, net_dns_source_t source);
static void net_dns_source_ns_dgram_input(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

static int net_dns_source_ns_dgram_output(
    net_dns_manage_t manage, net_dns_source_ns_t ns, void const * data, uint16_t buf_size);

static int net_dns_source_ns_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static void net_dns_source_ns_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static int net_dns_source_ns_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
static void net_dns_source_ns_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

static char const * net_dns_source_ns_req_print_name(
    net_dns_manage_t manage, write_stream_t ws, char const * p, char const * buf, uint32_t buf_size, uint8_t level);
static const char * net_dns_source_ns_req_dump(
    net_dns_manage_t manage, mem_buffer_t buffer, char const * buf, uint32_t data_size);

net_dns_source_ns_t
net_dns_source_ns_create(net_dns_manage_t manage, net_driver_t driver, net_address_t addr, uint8_t is_own) {
    net_dns_source_t source =
        net_dns_source_create(
            manage,
            sizeof(struct net_dns_source_ns),
            net_dns_source_ns_init,
            net_dns_source_ns_fini,
            net_dns_source_ns_dump,
            sizeof(struct net_dns_source_ns_ctx),
            net_dns_source_ns_ctx_init,
            net_dns_source_ns_ctx_fini,
            net_dns_source_ns_ctx_start,
            net_dns_source_ns_ctx_cancel);
    if (source == NULL) return NULL;

    net_dns_source_ns_t ns = net_dns_source_data(source);

    ns->m_driver = driver;
    
    if (is_own) {
        ns->m_address = addr;
    }
    else {
        ns->m_address = net_address_copy(manage->m_schedule, addr);
        if (ns->m_address == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: ns: copy address fail!");
            net_dns_source_free(source);
            return NULL;
        }
    }

    if (net_address_port(ns->m_address) == 0) {
        net_address_set_port(ns->m_address, 53);
    }

    if (manage->m_debug) {
        CPE_INFO(
            manage->m_em, "dns-cli: ns %s created!",
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), source));
    }
    
    return ns;
}

net_dns_source_ns_t net_dns_source_ns_find(net_dns_manage_t manage, net_address_t addr) {
    net_dns_source_t source;

    uint16_t port = net_address_port(addr);
    if (port == 0) port = 53; 
    
    TAILQ_FOREACH(source, &manage->m_sources, m_next) {
        if (source->m_init != net_dns_source_ns_init) continue;

        net_dns_source_ns_t ns = net_dns_source_data(source);

        if (net_address_cmp_without_port(ns->m_address, addr) != 0) continue;
        if (net_address_port(ns->m_address) != port) continue;

        return ns;
    }

    return 0;
}

void net_dns_source_ns_free(net_dns_source_ns_t ns) {
    net_dns_source_free(net_dns_source_from_data(ns));
}

int net_dns_source_ns_init(net_dns_source_t source) {
    net_dns_source_ns_t ns = net_dns_source_data(source);

    ns->m_driver = NULL;
    ns->m_address = NULL;
    ns->m_dgram = NULL;
    ns->m_endpoint = NULL;
    ns->m_retry_count = 0;
    ns->m_timeout_ms = 2000;
    ns->m_max_transaction = 0;
    
    return 0;
}

void net_dns_source_ns_fini(net_dns_source_t source) {
    net_dns_source_ns_t ns = net_dns_source_data(source);
    net_dns_manage_t manage = net_dns_source_manager(source);

    if (manage->m_debug) {
        CPE_INFO(
            manage->m_em, "dns-cli: ns %s free!",
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), source));
    }
    
    if (ns->m_address) {
        net_address_free(ns->m_address);
        ns->m_address = NULL;
    }

    if (ns->m_dgram) {
        net_dgram_free(ns->m_dgram);
        ns->m_dgram = NULL;
    }
    
    if (ns->m_endpoint) {
        net_endpoint_free(ns->m_endpoint);
        ns->m_endpoint = NULL;
    }
}

void net_dns_source_ns_dump(write_stream_t ws, net_dns_source_t source) {
    net_dns_source_ns_t ns = net_dns_source_data(source);
    
    stream_printf(ws, "ns[");
    if (ns->m_address) {
        net_address_print(ws, ns->m_address);
    }
    stream_printf(ws, "]");
}

int net_dns_source_ns_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    struct net_dns_source_ns_ctx * ns_ctx = net_dns_task_ctx_data(task_ctx);
    net_dns_source_ns_t ns = net_dns_source_data(source);
    
    net_dns_task_ctx_set_retry_count(task_ctx, ns->m_retry_count);
    net_dns_task_ctx_set_timeout(task_ctx, ns->m_timeout_ms);

    ns_ctx->m_transaction = ++ns->m_max_transaction;
    
    return 0;
}

void net_dns_source_ns_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}

int net_dns_source_ns_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    net_dns_task_t task = net_dns_task_ctx_task(task_ctx);
    net_dns_manage_t manage = net_dns_task_ctx_manage(task_ctx);
    net_dns_source_ns_t ns = net_dns_source_data(source);
    struct net_dns_source_ns_ctx * ns_ctx = net_dns_task_ctx_data(task_ctx);

    mem_buffer_t buffer = &manage->m_data_buffer;
    mem_buffer_clear_data(buffer);

    size_t buf_capacity = 1500;
    char * buf = mem_buffer_alloc(buffer, buf_capacity);
    if (buf == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: build ns req: alloc fail!");
        return -1; 
    }

    char *p = buf;

    /* head */
    CPE_COPY_HTON16(p, &ns_ctx->m_transaction); p+=2;

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
    
    /* query */
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
 
    uint16_t qtype = 1;
    CPE_COPY_HTON16(p, &qtype); p+=2;
    
    uint16_t qclass = 1;
    CPE_COPY_HTON16(p, &qclass); p+=2;

    uint16_t buf_size = (uint32_t)(p - buf);

    return net_dns_source_ns_dgram_output(manage, ns, buf, buf_size);
}

void net_dns_source_ns_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
}

static void net_dns_source_ns_dgram_input(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t from)
{
    net_dns_source_ns_t ns = ctx;
    net_dns_source_t source = net_dns_source_from_data(ns);
    net_dns_manage_t manage = net_dns_source_manager(source);

    if (data_size < 12) {
        CPE_ERROR(manage->m_em, "dns-cli: udp <-- size %d too small!", (int)data_size);
        return;
    }

    if (manage->m_debug >= 2) {
        CPE_INFO(
            manage->m_em, "dns-cli: udp <-- %s",
            net_dns_source_ns_req_dump(manage, net_dns_manage_tmp_buffer(manage), data, (uint32_t)data_size));
    }
    
    char const * p = data;

    uint16_t ident;
    CPE_COPY_NTOH16(&ident, p); p+=2;

    uint16_t flags;
    CPE_COPY_NTOH16(&flags, p); p+=2;

    uint16_t qdcount;
    CPE_COPY_NTOH16(&qdcount, p); p+=2;

    uint16_t ancount;
    CPE_COPY_NTOH16(&ancount, p); p+=2;

    uint16_t nscount;
    CPE_COPY_NTOH16(&nscount, p); p+=2;
 
    uint16_t arcount;
    CPE_COPY_NTOH16(&arcount, p); p+=2;

    uint16_t i;
    for(i = 0; i < qdcount; i++) {
        p = net_dns_source_ns_req_print_name(manage, NULL, p, data, (uint32_t)data_size, 0);
        p += 2 + 2; /*type + class*/
    }

    //uint8_t success_count = 0;

    int64_t expire_base_ms = cur_time_ms();
    mem_buffer_t buffer = &manage->m_data_buffer;
    for(i = 0; i < ancount; i++) {
        struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
        mem_buffer_clear_data(buffer);
        p = net_dns_source_ns_req_print_name(manage, (write_stream_t)&ws, p, data, (uint32_t)data_size, 0);
        stream_putc((write_stream_t)&ws, 0);

        const char * hostname = mem_buffer_make_continuous(buffer, 0);

        uint16_t res_type;
        CPE_COPY_NTOH16(&res_type, p); p+=2;

        uint16_t res_class;
        CPE_COPY_NTOH16(&res_class, p); p+=2;
        
        uint32_t ttl;
        CPE_COPY_NTOH32(&ttl, p); p+=4;

        uint16_t rdlength;
        CPE_COPY_NTOH16(&rdlength, p); p+=2;

        switch(res_type) {
        case 5: {
            net_dns_entry_t entry = net_dns_entry_find(manage, hostname);
            if (entry == NULL) {
                if (manage->m_debug) {
                    CPE_INFO(manage->m_em, "dns-cli: %s no entry!", hostname);
                }
                p += rdlength;
                continue;
            }

            mem_buffer_clear_data(buffer);
            p = net_dns_source_ns_req_print_name(manage, (write_stream_t)&ws, p, data, (uint32_t)data_size, 0);
            stream_putc((write_stream_t)&ws, 0);

            const char * cname = mem_buffer_make_continuous(buffer, 0);
            net_dns_entry_t cname_entry = net_dns_entry_find(manage, cname);
            if (cname_entry == NULL) {
                cname_entry = net_dns_entry_create(manage, cname);
                if (cname_entry == NULL) {
                    CPE_ERROR(manage->m_em, "dns-cli: %s create cname entry %s no entry!", net_dns_entry_hostname(entry), cname);
                    continue;
                }
            }
            net_dns_entry_set_main(cname_entry, entry);
            continue;
        }
        case 1: {
            break;
        }
        default:
            if (manage->m_debug) {
                CPE_INFO(manage->m_em, "dns-cli: udp <-- %s type %d not support, ignore record", hostname, res_type);
            }
            p += rdlength;
            continue;
        }

        net_address_t address = NULL;
        if (rdlength == 4) {
            struct net_address_data_ipv4 ipv4;
            ipv4.u8[0] = p[0];
            ipv4.u8[1] = p[1];
            ipv4.u8[2] = p[2];
            ipv4.u8[3] = p[3];
            address = net_address_create_from_data_ipv4(manage->m_schedule, &ipv4, 0);
            if (address == NULL) {
                CPE_ERROR(manage->m_em, "dns-cli: udp <-- create address for %s fail", hostname);
                p += rdlength;
                continue;
            }
        }
        else {
            if (manage->m_debug) {
                CPE_INFO(manage->m_em, "dns-cli: udp <-- %s answer length %d unknown, ignore record", hostname, rdlength);
            }
            p += rdlength;
            continue;
        }

        p += rdlength;

        net_dns_entry_t entry = net_dns_entry_find(manage, hostname);
        if (entry == NULL) {
            if (manage->m_debug) {
                CPE_INFO(manage->m_em, "dns-cli: %s no entry!", hostname);
            }
            net_address_free(address);
            continue;
        }

        net_dns_entry_item_t item = net_dns_entry_item_create(entry, source, address, 1, expire_base_ms + ttl);
        if (item == NULL) {
            net_address_free(address);
            continue;
        }

        net_dns_task_t task = net_dns_entry_task(net_dns_entry_effective(entry));
        if (task) {
            net_dns_task_step_t curent_step = net_dns_task_step_current(task);
            if (curent_step) {
                struct net_dns_task_ctx_it task_ctx_it;
                net_dns_task_ctx_t task_ctx;
                net_dns_task_step_ctxes(curent_step, &task_ctx_it);

                while((task_ctx = net_dns_task_ctx_it_next(&task_ctx_it))) {
                    if (net_dns_task_ctx_source(task_ctx) != source) continue;

                    struct net_dns_source_ns_ctx * ns_ctx = net_dns_task_ctx_data(task_ctx);
                    if (ns_ctx->m_transaction == ident) {
                        net_dns_task_ctx_set_success(task_ctx);
                        break;;
                    }
                }
            }
        }
    }
}

static char const *
net_dns_source_ns_req_print_name(
    net_dns_manage_t manage, write_stream_t ws, char const * p, char const * buf, uint32_t buf_size, uint8_t level)
{
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

            net_dns_source_ns_req_print_name(manage, ws, buf + offset, buf, buf_size, level + 1);

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

static void net_dns_source_ns_req_print(net_dns_manage_t manage, write_stream_t ws, char const * buf, uint32_t buf_size) {
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
        p = net_dns_source_ns_req_print_name(manage, ws, p, buf, buf_size, 0);

        uint16_t type;
        CPE_COPY_NTOH16(&type, p); p+=2;
        stream_printf(ws, ", type=%u",type);
        
        uint16_t class;
        CPE_COPY_NTOH16(&class, p); p+=2;
        stream_printf(ws, ", class=%u",class);
    }
 
    for(i = 0; i < ancount; i++) {
        stream_printf(ws, "\n    answer[%u]: ", i);
        p = net_dns_source_ns_req_print_name(manage, ws, p, buf, buf_size, 0);

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
            p = net_dns_source_ns_req_print_name(manage, ws, p, buf, buf_size, 0);
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

static const char * net_dns_source_ns_req_dump(net_dns_manage_t manage, mem_buffer_t buffer, char const * buf, uint32_t data_size) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    
    net_dns_source_ns_req_print(manage, (write_stream_t)&stream, buf, data_size);
    stream_putc((write_stream_t)&stream, 0);

    return mem_buffer_make_continuous(buffer, 0);
}

static int net_dns_source_ns_dgram_output(
    net_dns_manage_t manage, net_dns_source_ns_t ns, void const * buf, uint16_t buf_size)
{
    if (ns->m_dgram == NULL) {
        ns->m_dgram = net_dgram_create(
            ns->m_driver, NULL, net_dns_source_ns_dgram_input, ns);
        if (ns->m_dgram == NULL) {
            CPE_ERROR(
                manage->m_em, "dns-cli: ns[%s]: create dgram fail",
                net_address_dump(net_dns_manage_tmp_buffer(manage), ns->m_address));
            return -1;
        }
    }
    
    if (net_dgram_send(ns->m_dgram, ns->m_address, buf, buf_size) < 0) {
        CPE_ERROR(
            manage->m_em, "dns-cli: ns[%s]: send data fail",
            net_address_dump(net_dns_manage_tmp_buffer(manage), ns->m_address));
        return -1;
    }

    if (manage->m_debug >= 2) {
        char address_buf[128];
        snprintf(address_buf, sizeof(address_buf), "%s", net_address_dump(net_dns_manage_tmp_buffer(manage), ns->m_address));
        
        CPE_INFO(
            manage->m_em, "dns-cli: ns[%s]: udp --> %s",
            address_buf,
            net_dns_source_ns_req_dump(manage, net_dns_manage_tmp_buffer(manage), buf, buf_size));
    }

    return 0;
}
