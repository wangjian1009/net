#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/ringbuffer.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint_i.h"
#include "net_protocol_i.h"
#include "net_driver_i.h"
#include "net_schedule_i.h"
#include "net_address_i.h"
#include "net_link_i.h"
#include "net_dns_query_i.h"

static void net_endpoint_dns_query_callback(void * ctx, net_address_t address);

net_endpoint_t
net_endpoint_create(net_driver_t driver, net_endpoint_type_t type, net_protocol_t protocol) {
    net_schedule_t schedule = driver->m_schedule;
    net_endpoint_t endpoint;
    uint16_t capacity = sizeof(struct net_endpoint) + driver->m_endpoint_capacity + schedule->m_endpoint_protocol_capacity;
        
    endpoint = TAILQ_FIRST(&driver->m_free_endpoints);
    if (endpoint) {
        TAILQ_REMOVE(&driver->m_free_endpoints, endpoint, m_next_for_driver);
    }
    else {
        endpoint = mem_alloc(schedule->m_alloc, capacity);
        if (endpoint == NULL) {
            CPE_ERROR(schedule->m_em, "endpoint: alloc fail!");
            return NULL;
        }
    }

    bzero(endpoint, capacity);

    endpoint->m_driver = driver;
    endpoint->m_type = type;
    endpoint->m_address = NULL;
    endpoint->m_remote_address = NULL;
    endpoint->m_protocol = protocol;
    endpoint->m_link = NULL;
    endpoint->m_id = schedule->m_endpoint_max_id + 1;
    endpoint->m_state = net_endpoint_state_disable;
    endpoint->m_dns_query = NULL;
    endpoint->m_rb = NULL;
    endpoint->m_wb = NULL;
    endpoint->m_fb = NULL;
    
    if (protocol->m_endpoint_init(endpoint) != 0) {
        TAILQ_INSERT_TAIL(&driver->m_free_endpoints, endpoint, m_next_for_driver);
        return NULL;
    }

    if (driver->m_endpoint_init(endpoint) != 0) {
        protocol->m_endpoint_fini(endpoint);
        TAILQ_INSERT_TAIL(&driver->m_free_endpoints, endpoint, m_next_for_driver);
        return NULL;
    }

    cpe_hash_entry_init(&endpoint->m_hh);
    if (cpe_hash_table_insert_unique(&schedule->m_endpoints, endpoint) != 0) {
        CPE_ERROR(schedule->m_em, "endpoint: id duplicate!");
        driver->m_endpoint_fini(endpoint);
        protocol->m_endpoint_fini(endpoint);
        TAILQ_INSERT_TAIL(&driver->m_free_endpoints, endpoint, m_next_for_driver);
        return NULL;
    }

    schedule->m_endpoint_max_id++;
    TAILQ_INSERT_TAIL(&driver->m_endpoints, endpoint, m_next_for_driver);

    if (schedule->m_debug >= 2) {
        CPE_INFO(schedule->m_em, "core: %s created!", net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
    }
    
    return endpoint;
}

void net_endpoint_free(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (schedule->m_debug >= 2) {
        CPE_INFO(schedule->m_em, "core: %s free!", net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
    }

    if (endpoint->m_dns_query) {
        net_dns_query_free(endpoint->m_dns_query);
        endpoint->m_dns_query = NULL;
    }
    
    if (endpoint->m_link) {
        if (endpoint->m_link->m_local == endpoint) {
            endpoint->m_link->m_local_is_tie = 0;
        }
        else if (endpoint->m_link->m_remote == endpoint) {
            endpoint->m_link->m_remote_is_tie = 0;
        }
        net_link_free(endpoint->m_link);
        assert(endpoint->m_link == NULL);
    }
    
    endpoint->m_driver->m_endpoint_fini(endpoint);
    endpoint->m_protocol->m_endpoint_fini(endpoint);

    if (endpoint->m_rb) {
        assert(endpoint->m_rb->id == endpoint->m_id);
        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_rb);
        endpoint->m_rb = NULL;
    }

    if (endpoint->m_wb) {
        assert(endpoint->m_wb->id == endpoint->m_id);
        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_wb);
        endpoint->m_wb = NULL;
    }

    if (endpoint->m_fb) {
        assert(endpoint->m_fb->id == endpoint->m_id);
        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_fb);
        endpoint->m_fb = NULL;
    }
    
    if (endpoint->m_address) {
        net_address_free(endpoint->m_address);
        endpoint->m_address = NULL;
    }

    if (endpoint->m_remote_address) {
        net_address_free(endpoint->m_remote_address);
        endpoint->m_remote_address = NULL;
    }
    
    cpe_hash_table_remove_by_ins(&endpoint->m_driver->m_schedule->m_endpoints, endpoint);

    TAILQ_REMOVE(&endpoint->m_driver->m_endpoints, endpoint, m_next_for_driver);
    TAILQ_INSERT_TAIL(&endpoint->m_driver->m_free_endpoints, endpoint, m_next_for_driver);
}

void net_endpoint_real_free(net_endpoint_t endpoint) {
    net_driver_t driver = endpoint->m_driver;
    
    TAILQ_REMOVE(&driver->m_free_endpoints, endpoint, m_next_for_driver);
    mem_free(driver->m_schedule->m_alloc, endpoint);
}

net_schedule_t net_endpoint_schedule(net_endpoint_t endpoint) {
    return endpoint->m_protocol->m_schedule;
}

net_endpoint_type_t net_endpoint_type(net_endpoint_t endpoint) {
    return endpoint->m_type;
}

net_protocol_t net_endpoint_protocol(net_endpoint_t endpoint) {
    return endpoint->m_protocol;
}

net_driver_t net_endpoint_driver(net_endpoint_t endpoint) {
    return endpoint->m_driver;
}

uint32_t net_endpoint_id(net_endpoint_t endpoint) {
    return endpoint->m_id;
}

net_endpoint_t net_endpoint_find(net_schedule_t schedule, uint32_t id) {
    struct net_endpoint key;
    key.m_id = id;
    return cpe_hash_table_find(&schedule->m_endpoints, &key);
}

net_endpoint_state_t net_endpoint_state(net_endpoint_t endpoint) {
    return endpoint->m_state;
}

void net_endpoint_set_state(net_endpoint_t endpoint, net_endpoint_state_t state) {
    if (endpoint->m_state == state) return;
    
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    if (schedule->m_debug >= 2) {
        CPE_INFO(
            schedule->m_em, "core: %s: state %s ==> %s",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
            net_endpoint_state_str(endpoint->m_state),
            net_endpoint_state_str(state));
    }
    
    endpoint->m_state = state;
}

net_address_t net_endpoint_address(net_endpoint_t endpoint) {
    return endpoint->m_address;
}

int net_endpoint_set_address(net_endpoint_t endpoint, net_address_t address, uint8_t is_own) {
    if (endpoint->m_address) {
        net_address_free(endpoint->m_address);
    }

    if (is_own) {
        endpoint->m_address = address;
    }
    else {
        endpoint->m_address = net_address_copy(endpoint->m_driver->m_schedule, address);
        if (endpoint->m_address == NULL) return -1;
    }

    return 0;
}

net_address_t net_endpoint_remote_address(net_endpoint_t endpoint) {
    return endpoint->m_remote_address;
}

int net_endpoint_set_remote_address(net_endpoint_t endpoint, net_address_t address, uint8_t is_own) {
    if (endpoint->m_remote_address) {
        if (endpoint->m_dns_query) {
            net_dns_query_free(endpoint->m_dns_query);
            endpoint->m_dns_query = NULL;
        }

        net_address_free(endpoint->m_remote_address);
    }

    if (is_own) {
        endpoint->m_remote_address = address;
    }
    else {
        endpoint->m_remote_address = net_address_copy(endpoint->m_driver->m_schedule, address);
        if (endpoint->m_remote_address == NULL) return -1;
    }

    return 0;
}

void * net_endpoint_data(net_endpoint_t endpoint) {
    return endpoint + 1;
}

net_endpoint_t net_endpoint_from_data(void * data) {
    return ((net_endpoint_t)data) - 1;
}

net_link_t net_endpoint_link(net_endpoint_t endpoint) {
    return endpoint->m_link;
}

net_endpoint_t net_endpoint_other(net_endpoint_t endpoint) {
    net_link_t link = endpoint->m_link;
    
    if (link == NULL) return NULL;

    return link->m_local == endpoint
        ? link->m_remote
        : link->m_remote == endpoint
        ? link->m_local
        : NULL;
}

int net_endpoint_connect(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (endpoint->m_state != net_endpoint_state_disable) {
        CPE_ERROR(
            schedule->m_em, "%s: connect: current state is %s, can`t connect!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
            net_endpoint_state_str(endpoint->m_state));
        return -1;
    }
    
    if (endpoint->m_remote_address == NULL) {
        CPE_ERROR(
            schedule->m_em, "%s: connect: no remote address!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        return -1;
    }

    if (net_address_resolved(endpoint->m_remote_address) == NULL) {
        if (net_schedule_dns_resolver(schedule) == NULL) {
            CPE_ERROR(
                schedule->m_em, "%s: connect: no dns resolver!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
            return -1;
        }

        endpoint->m_dns_query =
            net_dns_query_create(
                schedule, (const char *)net_address_data(endpoint->m_remote_address),
                net_endpoint_dns_query_callback, NULL, endpoint);
        if (endpoint->m_dns_query == NULL) {
            CPE_ERROR(
                schedule->m_em, "%s: connect: create dns query fail!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
            return -1;
        }

        net_endpoint_set_state(endpoint, net_endpoint_state_resolving);
        return 0;
    }
    else {
        return endpoint->m_driver->m_endpoint_connect(endpoint);
    }
}

void net_endpoint_disable(net_endpoint_t endpoint) {
    return endpoint->m_driver->m_endpoint_close(endpoint);
}

ringbuffer_block_t net_endpoint_common_buf_alloc(net_endpoint_t endpoint, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    ringbuffer_block_t blk;

    assert(size > 0);

    if (schedule->m_endpoint_tb) {
        ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
        schedule->m_endpoint_tb = NULL;
    }

    blk = ringbuffer_alloc(schedule->m_endpoint_buf, size);
    while (blk == NULL) {
        int collect_id = ringbuffer_collect(schedule->m_endpoint_buf);
        if(collect_id < 0) {
            CPE_ERROR(
                schedule->m_em, "%s: buf alloc: not enouth capacity, len=%d!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint), size);
            return NULL;
        }

        if ((uint32_t)collect_id == endpoint->m_id) {
            CPE_ERROR(
                schedule->m_em, "%s: buf alloc: self use block, require len %d",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint), size);

            CPE_ERROR(
                schedule->m_em, "dump buf:\n%s",
                ringbuffer_dump(net_schedule_tmp_buffer(schedule), schedule->m_endpoint_buf));
        
            return NULL;
        }
        
        net_endpoint_t free_endpoint = net_endpoint_find(schedule, (uint32_t)collect_id);
        assert(free_endpoint);
        assert(free_endpoint != endpoint);
        free_endpoint->m_rb = NULL;
        free_endpoint->m_wb = NULL;
        free_endpoint->m_fb = NULL;

        char free_endpoint_name[128];
        cpe_str_dup(free_endpoint_name, sizeof(free_endpoint_name), net_endpoint_dump(&schedule->m_tmp_buffer, free_endpoint));
        CPE_ERROR(
            schedule->m_em, "%s: buf alloc: not enouth free buff, free endpoint %s!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint), free_endpoint_name);
        net_endpoint_free(free_endpoint);
        
        blk = ringbuffer_alloc(schedule->m_endpoint_buf , size);
    }

    return blk;
}

uint32_t net_endpoint_hash(net_endpoint_t o) {
    return o->m_id;
}

int net_endpoint_eq(net_endpoint_t l, net_endpoint_t r) {
    return l->m_id == r->m_id;
}

void net_endpoint_print(write_stream_t ws, net_endpoint_t endpoint) {
    stream_printf(ws, "[%d: %s", endpoint->m_id, endpoint->m_protocol->m_name);
    if (endpoint->m_remote_address) {
        stream_printf(ws, "->");
        net_address_print(ws, endpoint->m_remote_address);
    }
    else if (endpoint->m_address) {
        stream_printf(ws, "@");
        net_address_print(ws, endpoint->m_address);
    }
    stream_printf(ws, "]");
}

const char * net_endpoint_dump(mem_buffer_t buff, net_endpoint_t endpoint) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buff);

    mem_buffer_clear_data(buff);
    net_endpoint_print((write_stream_t)&stream, endpoint);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buff, 0);
}

void * net_endpoint_protocol_data(net_endpoint_t endpoint) {
    return ((char*)(endpoint + 1)) + endpoint->m_driver->m_endpoint_capacity;
}

int net_endpoint_forward(net_endpoint_t endpoint) {
    net_endpoint_t other = net_endpoint_other(endpoint);
    return other ? other->m_protocol->m_endpoint_forward(other, endpoint) : 0;
}

const char * net_endpoint_state_str(net_endpoint_state_t state) {
    switch(state) {
    case net_endpoint_state_disable:
        return "disable";
    case net_endpoint_state_resolving:
        return "resolve";
    case net_endpoint_state_connecting:
        return "connecting";
    case net_endpoint_state_established:
        return "established";
    case net_endpoint_state_error:
        return "error";
    }
}

static void net_endpoint_dns_query_callback(void * ctx, net_address_t address) {
    net_endpoint_t endpoint = ctx;
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    endpoint->m_dns_query = NULL;
    
    if (address == NULL) {
        CPE_ERROR(
            schedule->m_em, "%s: resolve: dns resolve fail!!!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        net_endpoint_set_state(endpoint, net_endpoint_state_error);
        return;
    }

    if (net_address_set_resolved(endpoint->m_remote_address, address, 0) != 0) {
        CPE_ERROR(
            schedule->m_em, "%s: resolve: set resolve result fail!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        net_endpoint_set_state(endpoint, net_endpoint_state_error);
        return;
    }

    if (schedule->m_debug >= 2) {
        CPE_INFO(
            schedule->m_em, "%s: resolve: success!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
    }
    
    if (endpoint->m_driver->m_endpoint_connect(endpoint) != 0) {
        net_endpoint_set_state(endpoint, net_endpoint_state_error);
        return;
    }
}

