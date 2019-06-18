#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "net_dns_query.h"
#include "net_address.h"
#include "net_dns_svr_query_entry_i.h"

static void net_dns_svr_query_entry_callback(void * ctx, net_address_t main_address, net_address_it_t all_address);

net_dns_svr_query_entry_t
net_dns_svr_query_entry_create(net_dns_svr_query_t query, net_address_t address_own, net_dns_svr_query_entry_type_t type) {
    net_dns_svr_t dns_svr = query->m_itf->m_svr;
    
    net_dns_svr_query_entry_t query_entry = TAILQ_FIRST(&dns_svr->m_free_query_entries);
    if (query_entry) {
        TAILQ_REMOVE(&dns_svr->m_free_query_entries, query_entry, m_next);
    }
    else {
        query_entry = mem_alloc(dns_svr->m_alloc, sizeof(struct net_dns_svr_query_entry));
        if (query_entry == NULL) {
            CPE_ERROR(dns_svr->m_em, "dns-svr: query_entry alloc fail!");
            return NULL;
        }
    }

    query_entry->m_query = query;
    query_entry->m_type = type;
    query_entry->m_address = address_own;
    query_entry->m_result_count = 0;
    query_entry->m_local_query = NULL;
    query_entry->m_cache_name_offset = 0;

    TAILQ_INSERT_TAIL(&query->m_entries, query_entry, m_next);

    return query_entry;
}

void net_dns_svr_query_entry_free(net_dns_svr_query_entry_t query_entry) {
    net_dns_svr_t dns_svr = query_entry->m_query->m_itf->m_svr;

    if (query_entry->m_local_query) {
        net_dns_query_free(query_entry->m_local_query);
        query_entry->m_local_query = NULL;
        assert(query_entry->m_query->m_runing_entry_count > 0);
        query_entry->m_query->m_runing_entry_count--;
    }

    uint8_t i;
    for(i = 0; i < query_entry->m_result_count; ++i) {
        net_address_free(query_entry->m_results[i]);
    }
    query_entry->m_result_count = 0;

    net_address_free(query_entry->m_address);
    query_entry->m_address = NULL;
    
    TAILQ_REMOVE(&query_entry->m_query->m_entries, query_entry, m_next);

    query_entry->m_query = (net_dns_svr_query_t)dns_svr;
    TAILQ_INSERT_TAIL(&dns_svr->m_free_query_entries, query_entry, m_next);
}

void net_dns_svr_query_entry_real_free(net_dns_svr_query_entry_t query_entry) {
    net_dns_svr_t dns_svr = (net_dns_svr_t)query_entry->m_query;

    TAILQ_REMOVE(&dns_svr->m_free_query_entries, query_entry, m_next);
    mem_free(dns_svr->m_alloc, query_entry);
}

int net_dns_svr_query_entry_start(net_dns_svr_query_entry_t query_entry) {
    net_dns_svr_itf_t itf = query_entry->m_query->m_itf;
    net_dns_svr_t svr = itf->m_svr;
    
    assert(query_entry->m_local_query == NULL);

    net_dns_query_type_t query_type;
    switch(query_entry->m_type) {
    case net_dns_svr_query_entry_type_ipv4:
        query_type = net_dns_query_ipv4;
        break;
    case net_dns_svr_query_entry_type_ipv6:
        query_type = net_dns_query_ipv6;
        break;
    case net_dns_svr_query_entry_type_ptr:
        query_type = net_dns_query_domain;
        break;
    }

    /*启动查询 */
    query_entry->m_local_query =
        net_dns_query_create(
            svr->m_schedule,
            query_entry->m_address, query_type,
            itf->m_query_policy,
            net_dns_svr_query_entry_callback, NULL, query_entry);
    if (query_entry->m_local_query == NULL) {
        CPE_ERROR(svr->m_em, "dns-svr: start local query fail!");
        return -1;
    }
    query_entry->m_query->m_runing_entry_count++;

    return 0;
}

static void net_dns_svr_query_entry_callback(void * ctx, net_address_t main_address, net_address_it_t all_address) {
    net_dns_svr_query_entry_t query_entry = ctx;
    net_dns_svr_query_t query = query_entry->m_query;
    net_dns_svr_itf_t itf = query->m_itf;
    net_dns_svr_t svr = itf->m_svr;
    net_address_t result;
    
    assert(query_entry->m_local_query);
    query_entry->m_local_query = NULL;
    assert(query_entry->m_query->m_runing_entry_count > 0);
    query_entry->m_query->m_runing_entry_count--;

    while((result = net_address_it_next(all_address))) {
        switch(query_entry->m_type) {
        case net_dns_svr_query_entry_type_ipv4:
            if (net_address_type(result) != net_address_ipv4) continue;
            break;
        case net_dns_svr_query_entry_type_ipv6:
            if (net_address_type(result) != net_address_ipv6) continue;
            break;
        case net_dns_svr_query_entry_type_ptr:
            if (net_address_type(result) != net_address_domain) continue;
            break;
        }
        
        if (net_dns_svr_query_entry_have_address(query_entry, result)) continue;

        if (query_entry->m_result_count + 1 >= CPE_ARRAY_SIZE(query_entry->m_results)) {
            if (svr->m_debug >= 2) {
                char address_buf[512];
                cpe_str_dup(address_buf, sizeof(address_buf), net_dns_svr_query_entry_dump(net_dns_svr_tmp_buffer(svr), query_entry));
                CPE_INFO(
                    svr->m_em, "dns-svr: query %s ==> %s, count=%d, overflow, ignore",
                    address_buf,
                    net_address_host(net_dns_svr_tmp_buffer(svr), result),
                    query_entry->m_result_count);
            }
            break;
        }

        query_entry->m_results[query_entry->m_result_count] = net_address_copy(svr->m_schedule, result);
        if (query_entry->m_results[query_entry->m_result_count] == NULL) {
            char address_buf[512];
            cpe_str_dup(address_buf, sizeof(address_buf), net_dns_svr_query_entry_dump(net_dns_svr_tmp_buffer(svr), query_entry));
            CPE_ERROR(
                svr->m_em, "dns-svr: query %s ==> %s, dup address error, ignore",
                address_buf, net_address_host(net_dns_svr_tmp_buffer(svr), result));
            continue;
        }

        if (svr->m_debug >= 2) {
            char address_buf[512];
            cpe_str_dup(address_buf, sizeof(address_buf), net_dns_svr_query_entry_dump(net_dns_svr_tmp_buffer(svr), query_entry));
            CPE_INFO(
                svr->m_em, "dns-svr: query %s ==> [%d]%s",
                address_buf,
                query_entry->m_result_count,
                net_address_host(net_dns_svr_tmp_buffer(svr), query_entry->m_results[query_entry->m_result_count]));
        }

        query_entry->m_result_count++;
    }

    if (svr->m_debug) {
        if (query_entry->m_result_count == 0) {
            CPE_INFO(
                svr->m_em, "dns-svr: query %s ==> none",
                net_dns_svr_query_entry_dump(net_dns_svr_tmp_buffer(svr), query_entry));
        }
    }

    if (query_entry->m_query->m_runing_entry_count == 0) {
        net_dns_svr_itf_send_response(itf, query);
        net_dns_svr_query_free(query);
    }
}

uint8_t net_dns_svr_query_entry_have_address(net_dns_svr_query_entry_t query_entry, net_address_t address) {
    uint8_t i;
    for(i = 0; i < query_entry->m_result_count; ++i) {
        if (net_address_cmp(query_entry->m_results[i], address) == 0) return 1;
    }

    return 0;
}

uint16_t net_dns_svr_query_entry_type_to_qtype(net_dns_svr_query_entry_type_t entry_type) {
    switch(entry_type) {
    case net_dns_svr_query_entry_type_ipv4:
        return 1;
    case net_dns_svr_query_entry_type_ipv6:
        return 28;
    case net_dns_svr_query_entry_type_ptr:
        return 12;
    }
}

const char * net_dns_svr_query_entry_dump(mem_buffer_t buffer, net_dns_svr_query_entry_t entry) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);

    net_address_print((write_stream_t)&stream, entry->m_address);

    switch(entry->m_type) {
    case net_dns_svr_query_entry_type_ipv4:
        stream_printf((write_stream_t)&stream, " ipv4");
        break;
    case net_dns_svr_query_entry_type_ipv6:
        stream_printf((write_stream_t)&stream, " ipv6");
        break;
    case net_dns_svr_query_entry_type_ptr:
        stream_printf((write_stream_t)&stream, " ptr");
        break;
    }

    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}
