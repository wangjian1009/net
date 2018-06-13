#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address_i.h"

struct net_address_ipv4v6 *
net_address_create_ipv4v6(net_schedule_t schedule, net_address_type_t type, uint16_t port) {
    struct net_address_ipv4v6 * address_ipv4v6
        = (struct net_address_ipv4v6 *)TAILQ_FIRST(&schedule->m_free_addresses);
    if (address_ipv4v6) {
        TAILQ_REMOVE(&schedule->m_free_addresses, (struct net_address_in_cache *)address_ipv4v6, m_next);
    }
    else {
        address_ipv4v6 = mem_alloc(schedule->m_alloc, sizeof(struct net_address_ipv4v6));
        if (address_ipv4v6 == NULL) {
            CPE_ERROR(schedule->m_em, "net_address_create_ipv4v6: alloc fail!");
            return NULL;
        }
    }

    address_ipv4v6->m_schedule = schedule;
    address_ipv4v6->m_type = type;
    address_ipv4v6->m_port = port;

    return address_ipv4v6;
}

net_address_t net_address_create_ipv4(net_schedule_t schedule, const char * addr, uint16_t port) {
    struct net_address_data_ipv4 ipv4;
    ipv4.u32 = inet_addr(addr);

    if (ipv4.u32 == INADDR_NONE) {
        CPE_ERROR(schedule->m_em, "net_address_create_ipv4: ip %s format fail!", addr);
        return NULL;
    }
    
    struct net_address_ipv4v6 * addr_ipv4v6 = net_address_create_ipv4v6(schedule, net_address_ipv4, port);
    if (addr_ipv4v6 == NULL) return NULL;
    
    addr_ipv4v6->m_ipv4 = ipv4;

    return (net_address_t)addr_ipv4v6;
}

net_address_t net_address_create_from_data_ipv4(net_schedule_t schedule, net_address_data_ipv4_t addr_data, uint16_t port) {
    struct net_address_ipv4v6 * addr_ipv4v6 = net_address_create_ipv4v6(schedule, net_address_ipv4, port);
    if (addr_ipv4v6 == NULL) return NULL;
    addr_ipv4v6->m_ipv4 = *addr_data;
    return (net_address_t)addr_ipv4v6;
}

net_address_t net_address_create_from_data_ipv6(net_schedule_t schedule, net_address_data_ipv6_t addr_data, uint16_t port) {
    struct net_address_ipv4v6 * addr_ipv4v6 = net_address_create_ipv4v6(schedule, net_address_ipv6, port);
    if (addr_ipv4v6 == NULL) return NULL;
    addr_ipv4v6->m_ipv6 = *addr_data;
    return (net_address_t)addr_ipv4v6;
}

net_address_t
net_address_create_from_sockaddr(net_schedule_t schedule, struct sockaddr * addr, socklen_t addr_len) {
    if (addr->sa_family == AF_INET) {
        if (addr_len < sizeof(struct sockaddr_in)) {
            CPE_ERROR(
                schedule->m_em, "net_address_from_sockaddr: add len too small, expect %d, but %d!",
                (int)sizeof(struct sockaddr_in), (int)addr_len);
            return NULL;
        }

        struct sockaddr_in *s = (struct sockaddr_in *)addr;
        struct net_address_ipv4v6 * addr_ipv4v6 =
            net_address_create_ipv4v6(schedule, net_address_ipv4, ntohs(s->sin_port));
        if (addr_ipv4v6 == NULL) return NULL;

        addr_ipv4v6->m_ipv4.u32 = s->sin_addr.s_addr;
        return (net_address_t)addr_ipv4v6;
    }
    else if (addr->sa_family == AF_INET6) {
        if (addr_len < sizeof(struct sockaddr_in6)) {
            CPE_ERROR(
                schedule->m_em, "net_address_from_sockaddr: add len too small, expect %d, but %d!",
                (int)sizeof(struct sockaddr_in6), (int)addr_len);
            return NULL;
        }

        struct sockaddr_in6 *s = (struct sockaddr_in6 *)addr;
        struct net_address_ipv4v6 * addr_ipv4v6 =
            net_address_create_ipv4v6(schedule, net_address_ipv6, ntohs(s->sin6_port));
        if (addr_ipv4v6 == NULL) return NULL;

        memcpy(&addr_ipv4v6->m_ipv6, &s->sin6_addr, sizeof(addr_ipv4v6->m_ipv6));
        return (net_address_t)addr_ipv4v6;
    }
    else { 
        CPE_ERROR(schedule->m_em, "net_address_from_sockaddr: not support family %d!", addr->sa_family);
        return NULL;
    }
}

int net_address_to_sockaddr(net_address_t address, struct sockaddr * addr, socklen_t * addr_len) {
    switch(address->m_type) {
    case net_address_domain:
        CPE_ERROR(address->m_schedule->m_em, "net_address_to_sockaddr: not support domain!");
        return -1;
    case net_address_ipv4: {
        if (*addr_len < sizeof(struct sockaddr_in)) {
            CPE_ERROR(
                address->m_schedule->m_em, "net_address_to_sockaddr: add len too small, expect %d, but %d!",
                (int)sizeof(struct sockaddr_in), (int)*addr_len);
            return -1;
        }

        struct net_address_ipv4v6 * address_ipv4v6 = (struct net_address_ipv4v6 *)address;
        struct sockaddr_in *s = (struct sockaddr_in *)addr;

        bzero(s, sizeof(*s));
        s->sin_family = AF_INET;
        s->sin_port = htons(address_ipv4v6->m_port);
        s->sin_addr.s_addr = address_ipv4v6->m_ipv4.u32;
        *addr_len = sizeof(*s);
        
        return 0;
    }
    case net_address_ipv6: {
        if (*addr_len < sizeof(struct sockaddr_in6)) {
            CPE_ERROR(
                address->m_schedule->m_em, "net_address_to_sockaddr: add len too small, expect %d, but %d!",
                (int)sizeof(struct sockaddr_in6), (int)*addr_len);
            return -1;
        }

        struct net_address_ipv4v6 * address_ipv4v6 = (struct net_address_ipv4v6 *)address;
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)addr;
        
        bzero(s, sizeof(*s));
        s->sin6_family = AF_INET6;
        s->sin6_port = htons(address_ipv4v6->m_port);
        memcpy(&s->sin6_addr, &address_ipv4v6->m_ipv6, sizeof(address_ipv4v6->m_ipv6));
        *addr_len = sizeof(*s);
        
        return 0;
    }
    }
}

net_address_t
net_address_create_domain(net_schedule_t schedule, const char * url, uint16_t port, net_address_t resolved) {
    return net_address_create_domain_with_len(schedule, url, strlen(url), port, resolved);
}

net_address_t
net_address_create_domain_with_len(
    net_schedule_t schedule, const void * url, uint16_t url_len, uint16_t port, net_address_t resolved)
{
    struct net_address_domain * address_domain;
    
    address_domain = mem_alloc(schedule->m_alloc, sizeof(struct net_address_domain) + url_len + 1);
    if (address_domain == NULL) {
        CPE_ERROR(schedule->m_em, "net_address_create_domain: alloc fail, url-len=%d!", url_len);
        return NULL;
    }

    address_domain->m_schedule = schedule;
    address_domain->m_type = net_address_domain;
    address_domain->m_port = port;
    address_domain->m_resolved = resolved;
    memcpy(address_domain->m_url, url, url_len);
    address_domain->m_url[url_len] = 0;
    
    return (net_address_t)address_domain;
}

net_address_t net_address_copy(net_schedule_t schedule, net_address_t from) {
    if (from->m_type == net_address_domain) {
        struct net_address_domain * from_domain = (struct net_address_domain *)from;

        net_address_t dup_resolved = NULL;
        if (from_domain->m_resolved) {
            dup_resolved = net_address_copy(schedule, from_domain->m_resolved);
            if (dup_resolved == NULL) return NULL;
        }

        net_address_t dup = net_address_create_domain(schedule, from_domain->m_url, from_domain->m_port, dup_resolved);
        if (dup == NULL) {
            net_address_free(dup_resolved);
            return NULL;
        }

        return dup;
    }
    else {
        struct net_address_ipv4v6 * from_ipv4v6 = (struct net_address_ipv4v6 *)from;
        
        struct net_address_ipv4v6 * address_ipv4v6 = 
            net_address_create_ipv4v6(schedule, from->m_type, from->m_port);
        if (address_ipv4v6 == NULL) return NULL;

        if (from->m_type == net_address_ipv4) {
            address_ipv4v6->m_ipv4 = from_ipv4v6->m_ipv4;
        }
        else {
            address_ipv4v6->m_ipv6 = from_ipv4v6->m_ipv6;
        }
        
        return (net_address_t)address_ipv4v6;
    }
}

void net_address_free(net_address_t address) {
    if (address->m_type == net_address_domain) {
        struct net_address_domain * address_domain = (struct net_address_domain *)address;
        if (address_domain->m_resolved) {
            net_address_free(address_domain->m_resolved);
        }
        mem_free(address->m_schedule->m_alloc, address_domain);
    }
    else {
        struct net_address_in_cache * address_in_cache = (struct net_address_in_cache *)address;
        TAILQ_INSERT_TAIL(&address_in_cache->m_schedule->m_free_addresses, address_in_cache, m_next);
    }
}

void net_address_real_free(net_address_in_cache_t address) {
    TAILQ_REMOVE(&address->m_schedule->m_free_addresses, address, m_next);
    mem_free(address->m_schedule->m_alloc, address);
}

net_address_type_t net_address_type(net_address_t address) {
    return address->m_type;
}

uint16_t net_address_port(net_address_t address) {
    return address->m_port;
}

void net_address_set_port(net_address_t address, uint16_t port) {
    address->m_port = port;
}

void const * net_address_data(net_address_t address) {
    switch(address->m_type) {
    case net_address_ipv4:
        return &((struct net_address_ipv4v6 *)address)->m_ipv4;
    case net_address_ipv6:
        return &((struct net_address_ipv4v6 *)address)->m_ipv6;
    case net_address_domain:
        return ((struct net_address_domain *)address)->m_url;
    }
}

int net_address_set_resolved(net_address_t address, net_address_t resolved, uint8_t is_own) {
    switch(address->m_type) {
    case net_address_ipv4:
    case net_address_ipv6:
        return -1;
    case net_address_domain: {
        if (resolved) {
            if (resolved->m_type != net_address_ipv4 && resolved->m_type != net_address_ipv6) {
                CPE_ERROR(address->m_schedule->m_em, "net_address_set_resolved: resolved address type is %d!", resolved->m_type);
                return -1;
            }
        }

        struct net_address_domain * domain_addr = (struct net_address_domain *)address;
        if (domain_addr->m_resolved) {
            net_address_free(domain_addr->m_resolved);
        }

        if (resolved == NULL) {
            domain_addr->m_resolved = NULL;
        }
        else {
            if (is_own) {
                domain_addr->m_resolved = resolved;
            }
            else {
                domain_addr->m_resolved = net_address_copy(address->m_schedule, resolved);
                if (domain_addr->m_resolved) {
                    CPE_ERROR(address->m_schedule->m_em, "net_address_set_resolved: address copy fail!");
                    return -1;
                }
            }

            domain_addr->m_resolved->m_port = domain_addr->m_port;
        }
        return 0;
    }
    }
}

net_address_t net_address_resolved(net_address_t address) {
    switch(address->m_type) {
    case net_address_ipv4:
    case net_address_ipv6:
        return address;
    case net_address_domain:
        return ((struct net_address_domain *)address)->m_resolved;
    }
}

void net_address_print(write_stream_t ws, net_address_t address) {
    switch(address->m_type) {
    case net_address_ipv4: {
        char buf[INET_ADDRSTRLEN] = { 0 };
        inet_ntop(AF_INET, &((struct net_address_ipv4v6 *)address)->m_ipv4, buf, sizeof(buf));
        stream_printf(ws, "%s:%d", buf, address->m_port);
        break;
    }
    case net_address_ipv6: {
        char buf[INET6_ADDRSTRLEN] = { 0 };
        inet_ntop(AF_INET6, &((struct net_address_ipv4v6 *)address)->m_ipv6, buf, sizeof(buf));
        stream_printf(ws, "%s:%d", buf, address->m_port);
        break;
    }
    case net_address_domain: {
        struct net_address_domain * address_domain = (struct net_address_domain *)address;
        stream_printf(ws, "%s:%d", address_domain->m_url, address_domain->m_port);
        break;
    }
    }
}

const char * net_address_dump(mem_buffer_t buffer, net_address_t address) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    
    net_address_print((write_stream_t)&stream, address);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

int net_address_cmp(net_address_t l, net_address_t r) {
    if (l->m_type != r->m_type) {
        return (int)l->m_type - (int)r->m_type;
    }

    if (l->m_port != r->m_port) {
        return (int)l->m_port - (int)r->m_port;
    }
    
    switch(l->m_type) {
    case net_address_domain:
        return strcmp(((struct net_address_domain *)l)->m_url, ((struct net_address_domain *)r)->m_url);
    case net_address_ipv4:
        return memcmp(&((struct net_address_ipv4v6 *)l)->m_ipv4, &((struct net_address_ipv4v6 *)r)->m_ipv4, sizeof(struct in_addr));
    case net_address_ipv6:
        return memcmp(&((struct net_address_ipv4v6 *)l)->m_ipv6, &((struct net_address_ipv4v6 *)r)->m_ipv6, sizeof(struct in6_addr));
    }
}

uint32_t net_address_hash(net_address_t address) {
    uint32_t r;

    switch(address->m_type) {
    case net_address_ipv4:
        r = ((struct net_address_ipv4v6 *)address)->m_ipv4.u32;
        CPE_HASH_UINT32(r);
        r ^= (uint32_t)address->m_port;
        break;
    case net_address_ipv6:
        break;
    case net_address_domain:
        break;
    }
        
    return r;
}
