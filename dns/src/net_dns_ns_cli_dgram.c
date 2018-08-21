#include "cpe/pal/pal_platform.h"
#include "cpe/utils/time_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address.h"
#include "net_dns_ns_cli_dgram_i.h"
#include "net_dns_ns_pro.h"
#include "net_dns_source_ns_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_task_i.h"
#include "net_dns_task_ctx_i.h"

void net_dns_ns_cli_dgram_input(
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
            net_dns_ns_req_dump(manage, net_dns_manage_tmp_buffer(manage), data, (uint32_t)data_size));
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
        p = net_dns_ns_req_print_name(manage, NULL, p, data, (uint32_t)data_size, 0);
        p += 2 + 2; /*type + class*/
    }

    //uint8_t success_count = 0;

    int64_t expire_base_ms = cur_time_ms();
    mem_buffer_t buffer = &manage->m_data_buffer;
    for(i = 0; i < ancount; i++) {
        struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
        mem_buffer_clear_data(buffer);
        p = net_dns_ns_req_print_name(manage, (write_stream_t)&ws, p, data, (uint32_t)data_size, 0);
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
            p = net_dns_ns_req_print_name(manage, (write_stream_t)&ws, p, data, (uint32_t)data_size, 0);
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
                        break;
                    }
                }
            }
        }

        net_address_free(address);
    }
}
