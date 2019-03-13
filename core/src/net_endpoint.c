#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/ringbuffer.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint_i.h"
#include "net_endpoint_monitor_i.h"
#include "net_debug_setup_i.h"
#include "net_protocol_i.h"
#include "net_driver_i.h"
#include "net_schedule_i.h"
#include "net_address_i.h"
#include "net_link_i.h"
#include "net_dns_query_i.h"

static void net_endpoint_dns_query_callback(void * ctx, net_address_t main_address, net_address_it_t it);
static int net_endpoint_notify_state_changed(net_endpoint_t endpoint, net_endpoint_state_t old_state);

net_endpoint_t
net_endpoint_create(net_driver_t driver, net_protocol_t protocol) {
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
    endpoint->m_address = NULL;
    endpoint->m_remote_address = NULL;
    endpoint->m_protocol = protocol;
    endpoint->m_prepare_connect = NULL;
    endpoint->m_prepare_connect_ctx = NULL;
    endpoint->m_close_after_send = 0;
    endpoint->m_protocol_debug = protocol->m_debug;
    endpoint->m_driver_debug = driver->m_debug;
    endpoint->m_rb_is_full = 0;
    endpoint->m_error_source = net_endpoint_error_source_network;
    endpoint->m_error_no = 0;
    endpoint->m_error_msg = NULL;
    endpoint->m_link = NULL;
    endpoint->m_id = schedule->m_endpoint_max_id + 1;
    endpoint->m_state = net_endpoint_state_disable;
    endpoint->m_dns_query = NULL;
    endpoint->m_tb = NULL;

    endpoint->m_all_buf_limit = NET_ENDPOINT_NO_LIMIT;
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
        endpoint->m_bufs[i].m_buf = NULL;
        endpoint->m_bufs[i].m_size = 0;
        endpoint->m_bufs[i].m_limit = NET_ENDPOINT_NO_LIMIT;
    }
    
    endpoint->m_data_watcher_ctx = NULL;
    endpoint->m_data_watcher_fun = NULL;
    endpoint->m_data_watcher_fini = NULL;

    TAILQ_INIT(&endpoint->m_monitors);

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

    if (endpoint->m_protocol_debug || endpoint->m_driver_debug || schedule->m_debug >= 2) {
        CPE_INFO(schedule->m_em, "core: %s created!", net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
    }
    
    return endpoint;
}

void net_endpoint_free(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (endpoint->m_state == net_endpoint_state_deleting) {
        TAILQ_REMOVE(&endpoint->m_driver->m_deleting_endpoints, endpoint, m_next_for_driver);
    }
    else {
        TAILQ_REMOVE(&endpoint->m_driver->m_endpoints, endpoint, m_next_for_driver);
    }

    if (endpoint->m_state == net_endpoint_state_established) {
        endpoint->m_state = net_endpoint_state_deleting;
    }
    
    if (endpoint->m_data_watcher_fini) {
        endpoint->m_data_watcher_fini(endpoint->m_data_watcher_ctx, endpoint);
    }
    endpoint->m_data_watcher_ctx = NULL;
    endpoint->m_data_watcher_fini = NULL;
    endpoint->m_data_watcher_fun = NULL;

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
	if (strlen(endpoint->m_protocol->m_name) >= 32) {
		// TODO:
		assert(0);
	}
	else {
		endpoint->m_protocol->m_endpoint_fini(endpoint);
	}

    if (endpoint->m_tb) {
        assert(endpoint->m_tb == schedule->m_endpoint_tb);
        ringbuffer_shrink(schedule->m_endpoint_buf, schedule->m_endpoint_tb, 0);
        endpoint->m_tb = schedule->m_endpoint_tb = NULL;
    }

    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
        if (endpoint->m_bufs[i].m_buf) {
            assert(endpoint->m_bufs[i].m_buf->id == endpoint->m_id);
            ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_bufs[i].m_buf);
            endpoint->m_bufs[i].m_buf = NULL;
            endpoint->m_bufs[i].m_size = 0;
        }
    }

    while(!TAILQ_EMPTY(&endpoint->m_monitors)) {
        net_endpoint_monitor_t monitor = TAILQ_FIRST(&endpoint->m_monitors);
        assert(!monitor->m_is_processing);
        assert(!monitor->m_is_free);
        net_endpoint_monitor_free(monitor);
    }

    if (endpoint->m_protocol_debug || endpoint->m_driver_debug || schedule->m_debug >= 2) {
        CPE_INFO(schedule->m_em, "core: %s free!", net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
    }

    if (endpoint->m_address) {
        net_address_free(endpoint->m_address);
        endpoint->m_address = NULL;
    }

    if (endpoint->m_remote_address) {
        net_address_free(endpoint->m_remote_address);
        endpoint->m_remote_address = NULL;
    }

    if (endpoint->m_error_msg) {
        mem_free(schedule->m_alloc, endpoint->m_error_msg);
        endpoint->m_error_msg = NULL;
    }
    
    cpe_hash_table_remove_by_ins(&schedule->m_endpoints, endpoint);

    TAILQ_INSERT_TAIL(&endpoint->m_driver->m_free_endpoints, endpoint, m_next_for_driver);
}

int net_endpoint_set_protocol(net_endpoint_t endpoint, net_protocol_t protocol) {
    net_protocol_t old_protocol = endpoint->m_protocol;

    endpoint->m_protocol->m_endpoint_fini(endpoint);

    endpoint->m_protocol = protocol;
    if (endpoint->m_protocol->m_endpoint_init(endpoint) != 0) goto SET_PROTOCOL_ERROR;

    if (protocol->m_debug > endpoint->m_protocol_debug) {
        endpoint->m_protocol_debug = protocol->m_debug;
    }
    
    return 0;

SET_PROTOCOL_ERROR:
    endpoint->m_protocol = old_protocol;
    endpoint->m_protocol->m_endpoint_init(endpoint);
    CPE_ERROR(
        endpoint->m_driver->m_schedule->m_em, "core: %s: set protocol %s fail!",
        net_endpoint_dump(&endpoint->m_driver->m_schedule->m_tmp_buffer, endpoint),
        protocol->m_name);
    return -1;
}

void net_endpoint_real_free(net_endpoint_t endpoint) {
    net_driver_t driver = endpoint->m_driver;
    
    TAILQ_REMOVE(&driver->m_free_endpoints, endpoint, m_next_for_driver);
    mem_free(driver->m_schedule->m_alloc, endpoint);
}

net_schedule_t net_endpoint_schedule(net_endpoint_t endpoint) {
    return endpoint->m_protocol->m_schedule;
}

net_protocol_t net_endpoint_protocol(net_endpoint_t endpoint) {
    return endpoint->m_protocol;
}

const char * net_endpoint_protocol_name(net_endpoint_t endpoint) {
    return endpoint->m_protocol->m_name;
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

uint8_t net_endpoint_close_after_send(net_endpoint_t endpoint) {
    return endpoint->m_close_after_send;
}

void net_endpoint_set_close_after_send(net_endpoint_t endpoint, uint8_t is_close_after_send) {
    endpoint->m_close_after_send = is_close_after_send;

    if (!net_endpoint_have_any_data(endpoint)) {
        if (endpoint->m_protocol_debug || endpoint->m_driver_debug) {
            net_schedule_t schedule = endpoint->m_driver->m_schedule;
            CPE_INFO(
                schedule->m_em, "core: %s: auto close on set close-after-send!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        }

        if (net_endpoint_set_state(endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
        }
    }
}

uint8_t net_endpoint_protocol_debug(net_endpoint_t endpoint) {
    return endpoint->m_protocol_debug;
}

void net_endpoint_set_protocol_debug(net_endpoint_t endpoint, uint8_t debug) {
    endpoint->m_protocol_debug = debug;
}

uint8_t net_endpoint_driver_debug(net_endpoint_t endpoint) {
    return endpoint->m_driver_debug;
}

void net_endpoint_set_driver_debug(net_endpoint_t endpoint, uint8_t debug) {
    endpoint->m_driver_debug = debug;
}

void net_endpoint_update_debug_info(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    net_debug_setup_t debug_setup;

    TAILQ_FOREACH(debug_setup, &schedule->m_debug_setups, m_next_for_schedule) {
        if (!net_debug_setup_check_endpoint(debug_setup, endpoint)) continue;

        if (debug_setup->m_protocol_debug > endpoint->m_protocol_debug) {
            endpoint->m_protocol_debug = debug_setup->m_protocol_debug;
        }

        if (debug_setup->m_driver_debug > endpoint->m_driver_debug) {
            endpoint->m_driver_debug = debug_setup->m_driver_debug;
        }
    }
}

net_endpoint_state_t net_endpoint_state(net_endpoint_t endpoint) {
    return endpoint->m_state;
}

int net_endpoint_set_prepare_connect(net_endpoint_t endpoint, net_endpoint_prepare_connect_fun_t fun, void * ctx) {
    if (net_endpoint_is_active(endpoint)) {
    }
        
    endpoint->m_prepare_connect = fun;
    endpoint->m_prepare_connect_ctx = ctx;
    return 0;
}

uint8_t net_endpoint_is_active(net_endpoint_t endpoint) {
    switch(endpoint->m_state) {
    case net_endpoint_state_disable:
    case net_endpoint_state_logic_error:
    case net_endpoint_state_network_error:
    case net_endpoint_state_deleting:
        return 0;
    default:
        return 1;
    }
}

int net_endpoint_set_state(net_endpoint_t endpoint, net_endpoint_state_t state) {
    if (endpoint->m_state == state) return 0;

    assert(endpoint->m_state != net_endpoint_state_deleting);
    if (endpoint->m_state == net_endpoint_state_deleting) {
        return 0;
    }

    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    if (endpoint->m_protocol_debug || endpoint->m_driver_debug || schedule->m_debug >= 2) {
        CPE_INFO(
            schedule->m_em, "core: %s: state %s ==> %s",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
            net_endpoint_state_str(endpoint->m_state),
            net_endpoint_state_str(state));
    }

    uint8_t old_is_active = net_endpoint_is_active(endpoint);
    net_endpoint_state_t old_state = endpoint->m_state;
    
    endpoint->m_state = state;

    if (old_is_active && !net_endpoint_is_active(endpoint)) {
        uint8_t i;
        for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
            if (endpoint->m_bufs[i].m_buf) {
                assert(endpoint->m_bufs[i].m_buf->id == endpoint->m_id);
                ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_bufs[i].m_buf);
                endpoint->m_bufs[i].m_buf = NULL;
                endpoint->m_bufs[i].m_size = 0;
            }
        }

        if (endpoint->m_dns_query) {
            net_dns_query_free(endpoint->m_dns_query);
            endpoint->m_dns_query = NULL;
        }
        
        endpoint->m_driver->m_endpoint_close(endpoint);

        if (endpoint->m_address) {
            net_address_free(endpoint->m_address);
            endpoint->m_address = NULL;
        }
    }

    if (state == net_endpoint_state_deleting) {
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
        
        TAILQ_REMOVE(&endpoint->m_driver->m_endpoints, endpoint, m_next_for_driver);
        TAILQ_INSERT_TAIL(&endpoint->m_driver->m_deleting_endpoints, endpoint, m_next_for_driver);
        net_schedule_start_delay_process(schedule);
        return 0;
    }
    
    if (state == net_endpoint_state_established) {
        endpoint->m_close_after_send = 0;
        endpoint->m_error_source = net_endpoint_error_source_network;
        endpoint->m_error_no = 0;
        if (endpoint->m_error_msg) {
            mem_free(schedule->m_alloc, endpoint->m_error_msg);
            endpoint->m_error_msg = NULL;
        }
    }
    
    return net_endpoint_notify_state_changed(endpoint, old_state);
}

net_address_t net_endpoint_address(net_endpoint_t endpoint) {
    return endpoint->m_address;
}

void net_endpoint_set_error(net_endpoint_t endpoint, net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    endpoint->m_error_source = error_source;
    endpoint->m_error_no = error_no;

    if (endpoint->m_error_msg) {
        mem_free(schedule->m_alloc, endpoint->m_error_msg);
        endpoint->m_error_msg = NULL;
    }
    
    endpoint->m_error_msg = msg ? cpe_str_mem_dup(schedule->m_alloc, msg) : NULL;
}

net_endpoint_error_source_t net_endpoint_error_source(net_endpoint_t endpoint) {
    return endpoint->m_error_source;
}

uint32_t net_endpoint_error_no(net_endpoint_t endpoint) {
    return endpoint->m_error_no;
}

const char * net_endpoint_error_msg(net_endpoint_t endpoint) {
    return endpoint->m_error_msg ? endpoint->m_error_msg : "";
}

uint8_t net_endpoint_have_error(net_endpoint_t endpoint) {
    return (endpoint->m_error_source == net_endpoint_error_source_network
            && ((net_endpoint_network_errno_t)endpoint->m_error_no) == net_endpoint_network_errno_none)
        ? 0
        : 1;
}

int net_endpoint_set_address(net_endpoint_t endpoint, net_address_t address, uint8_t is_own) {
    if (endpoint->m_address) {
        net_address_free(endpoint->m_address);
        endpoint->m_address = NULL;
    }

    if (address) {
        if (is_own) {
            endpoint->m_address = address;
        }
        else {
            endpoint->m_address = net_address_copy(endpoint->m_driver->m_schedule, address);
            if (endpoint->m_address == NULL) return -1;
        }

        net_endpoint_update_debug_info(endpoint);
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
        endpoint->m_remote_address = NULL;
    }

    if (address) {
        if (is_own) {
            endpoint->m_remote_address = address;
        }
        else {
            endpoint->m_remote_address = net_address_copy(endpoint->m_driver->m_schedule, address);
            if (endpoint->m_remote_address == NULL) return -1;
        }

        net_endpoint_update_debug_info(endpoint);
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

    if (net_endpoint_is_active(endpoint)) {
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

    if (endpoint->m_prepare_connect) {
        uint8_t do_connect = 0;
        if (endpoint->m_prepare_connect(endpoint->m_prepare_connect_ctx, endpoint, &do_connect) != 0) {
            CPE_ERROR(
                schedule->m_em, "%s: connect: external prepare fail!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
            return -1;
        }

        if (!do_connect) {
            if (!net_endpoint_is_active(endpoint)) {
                if (net_endpoint_set_state(endpoint, net_endpoint_state_established) != 0) {
                    CPE_ERROR(
                        schedule->m_em, "%s: connect: external prepare success, set state to established fail!",
                        net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
                    return -1;
                }
            }
            
            return 0;
        }
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
                schedule, (const char *)net_address_data(endpoint->m_remote_address), NULL,
                net_endpoint_dns_query_callback, NULL, endpoint);
        if (endpoint->m_dns_query == NULL) {
            CPE_ERROR(
                schedule->m_em, "%s: connect: create dns query fail!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
            return -1;
        }

        return net_endpoint_set_state(endpoint, net_endpoint_state_resolving);
    }
    else {
        return endpoint->m_driver->m_endpoint_connect(endpoint);
    }
}

int net_endpoint_direct(net_endpoint_t endpoint, net_address_t target_addr) {
    if (endpoint->m_protocol->m_endpoint_direct == NULL) {
        CPE_ERROR(
            endpoint->m_driver->m_schedule->m_em,
            "core: direct: protocol %s not support direct, can`t work with router!",
            endpoint->m_protocol->m_name);
        return -1;
    }

    return endpoint->m_protocol->m_endpoint_direct(endpoint, target_addr);
}

static int net_endpoint_common_buf_move(void * ctx, ringbuffer_t rb, ringbuffer_block_t old_block, ringbuffer_block_t new_block) {
    net_schedule_t schedule = ctx;

    int id = new_block->id;
    assert(id >= 0);
    
    net_endpoint_t endpoint = net_endpoint_find(schedule, (uint32_t)id);
    if (endpoint == NULL) {
        CPE_ERROR(schedule->m_em, "schedule: buf move: endpoint %d not exist", id);
        return -1;
    }

    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
        if (endpoint->m_bufs[i].m_buf == old_block) {
            endpoint->m_bufs[i].m_buf = new_block;
            return 0;
        }
    }
    
    CPE_ERROR(schedule->m_em, "schedule: buf move: endpoint %d no old buf", id);
    return -1;
}

ringbuffer_block_t net_endpoint_common_buf_alloc(net_endpoint_t endpoint, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    ringbuffer_block_t blk;

    assert(schedule->m_endpoint_tb == NULL);
    assert(endpoint->m_tb == NULL);
    
    blk = ringbuffer_alloc(schedule->m_endpoint_buf, size);
    if  (blk == NULL) {
        CPE_ERROR(
            schedule->m_em, "%s: buf alloc: not enouth capacity, len=%d!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint), size);

        CPE_ERROR(schedule->m_em, "ringbuffer: before gc: dump buf:");
        ringbuffer_dump(schedule->m_endpoint_buf);

        ringbuffer_gc(schedule->m_endpoint_buf, schedule, net_endpoint_common_buf_move);

        CPE_ERROR(schedule->m_em, "ringbuffer: after gc: dump buf:");
        ringbuffer_dump(schedule->m_endpoint_buf);

        blk = ringbuffer_alloc(schedule->m_endpoint_buf, size);
        if  (blk == NULL) {
            int collect_id = ringbuffer_collect(schedule->m_endpoint_buf);
            if(collect_id < 0) {
                CPE_ERROR(
                    schedule->m_em, "%s: buf alloc: not enouth capacity(after gc), len=%d!",
                    net_endpoint_dump(&schedule->m_tmp_buffer, endpoint), size);
                return NULL;
            }

            if ((uint32_t)collect_id == endpoint->m_id) {
                CPE_ERROR(
                    schedule->m_em, "%s: buf alloc: self use block, require len %d",
                    net_endpoint_dump(&schedule->m_tmp_buffer, endpoint), size);

                uint8_t i;
                for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
                    if (endpoint->m_bufs[i].m_buf == NULL) continue;

                    CPE_ERROR(
                        schedule->m_em, "%s: buf alloc: self use block, clear %s buf, size=%d",
                        net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
                        net_endpoint_buf_type_str(i), endpoint->m_bufs[i].m_size);
                
                    endpoint->m_bufs[i].m_buf = NULL;
                    endpoint->m_bufs[i].m_size = 0;
                }

                CPE_ERROR(schedule->m_em, "dump buf:");
                ringbuffer_dump(schedule->m_endpoint_buf);
        
                return NULL;
            }
        
            net_endpoint_t free_endpoint = net_endpoint_find(schedule, (uint32_t)collect_id);
            assert(free_endpoint);
            assert(free_endpoint != endpoint);

            if (free_endpoint->m_tb) {
                assert(free_endpoint->m_tb == schedule->m_endpoint_tb);
                net_endpoint_buf_release(free_endpoint);
            }
        
            uint8_t i;
            for(i = 0; i < CPE_ARRAY_SIZE(free_endpoint->m_bufs); ++i) {
                if (free_endpoint->m_bufs[i].m_buf == NULL) continue;
                ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_bufs[i].m_buf);
                free_endpoint->m_bufs[i].m_buf = NULL;
                free_endpoint->m_bufs[i].m_size = 0;
            }

            char free_endpoint_name[128];
            cpe_str_dup(free_endpoint_name, sizeof(free_endpoint_name), net_endpoint_dump(&schedule->m_tmp_buffer, free_endpoint));
            CPE_ERROR(
                schedule->m_em, "%s: buf alloc: not enouth free buff, free endpoint %s!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint), free_endpoint_name);

            CPE_ERROR(schedule->m_em, "dump buf:");
            ringbuffer_dump(schedule->m_endpoint_buf);

            if (net_endpoint_set_state(free_endpoint, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(free_endpoint, net_endpoint_state_deleting);
            }

            if (endpoint->m_state == net_endpoint_state_deleting || endpoint->m_state == net_endpoint_state_disable) return NULL;
        
            blk = ringbuffer_alloc(schedule->m_endpoint_buf , size);
        }
    }

    return blk;
}

uint32_t net_endpoint_hash(net_endpoint_t o, void * user_data) {
    return o->m_id;
}

int net_endpoint_eq(net_endpoint_t l, net_endpoint_t r, void * user_data) {
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
    if (other == NULL) {
        net_schedule_t schedule = endpoint->m_driver->m_schedule;
        CPE_ERROR(
            schedule->m_em, "%s: forward: no other endpoint",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        return -1;
    }

    if (other->m_state == net_endpoint_state_deleting) {
        net_schedule_t schedule = endpoint->m_driver->m_schedule;
        CPE_ERROR(
            schedule->m_em, "%s: forward: other endpoint is deleting",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        return -1;
    }
    
    return other->m_protocol->m_endpoint_forward(other, endpoint);
}

void net_endpoint_set_data_watcher(
    net_endpoint_t endpoint,
    void * watcher_ctx,
    net_endpoint_data_watch_fun_t watcher_fun,
    net_endpoint_data_watch_ctx_fini_fun_t watcher_ctx_fini)
{
    endpoint->m_data_watcher_ctx = watcher_ctx;
    endpoint->m_data_watcher_fun = watcher_fun;
    endpoint->m_data_watcher_fini = watcher_ctx_fini;
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
    case net_endpoint_state_logic_error:
        return "logic-error";
    case net_endpoint_state_network_error:
        return "network-error";
    case net_endpoint_state_deleting:
        return "deleting";
    }
}

const char * net_endpoint_buf_type_str(net_endpoint_buf_type_t buf_type) {
    switch(buf_type) {
    case net_ep_buf_read:
        return "read";
    case net_ep_buf_forward:
        return "forward";
    case net_ep_buf_write:
        return "write";
    case net_ep_buf_user1:
        return "user1";
    case net_ep_buf_user2:
        return "user2";
    case net_ep_buf_user3:
        return "user3";
    case net_ep_buf_count:
        return "buf-count";
    }
}

const char * net_endpoint_data_event_str(net_endpoint_data_event_t data_evt) {
    switch(data_evt) {
    case net_endpoint_data_supply:
        return "supply";
    case net_endpoint_data_consume:
        return "consume";
    }
}

static void net_endpoint_dns_query_callback(void * ctx, net_address_t address, net_address_it_t all_address) {
    net_endpoint_t endpoint = ctx;
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    endpoint->m_dns_query = NULL;
    
    if (address == NULL) {
        CPE_ERROR(
            schedule->m_em, "%s: resolve: dns resolve fail!!!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        if (net_endpoint_set_state(endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_free(endpoint);
        }
        return;
    }

    if (net_address_set_resolved(endpoint->m_remote_address, address, 0) != 0) {
        CPE_ERROR(
            schedule->m_em, "%s: resolve: set resolve result fail!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        if (net_endpoint_set_state(endpoint, net_endpoint_state_logic_error) != 0) {
            net_endpoint_free(endpoint);
        }
        return;
    }

    if (schedule->m_debug >= 2) {
        CPE_INFO(
            schedule->m_em, "%s: resolve: success!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
    }
    
    if (endpoint->m_driver->m_endpoint_connect(endpoint) != 0) {
        if (net_endpoint_set_state(endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_free(endpoint);
        }
        return;
    }
}

void net_endpoint_clear_monitor_by_ctx(net_endpoint_t endpoint, void * ctx) {
    net_endpoint_monitor_t monitor = TAILQ_FIRST(&endpoint->m_monitors);
    while(monitor) {
        net_endpoint_monitor_t next_monitor = TAILQ_NEXT(monitor, m_next);
        if (monitor->m_ctx == ctx) {
            net_endpoint_monitor_free(monitor);
        }
        monitor = next_monitor;
    }
}

static int net_endpoint_notify_state_changed(net_endpoint_t endpoint, net_endpoint_state_t old_state) {
    int rv = endpoint->m_protocol->m_endpoint_on_state_chagne
        ? endpoint->m_protocol->m_endpoint_on_state_chagne(endpoint, old_state)
        : net_endpoint_is_active(endpoint)
        ? 0
        : -1;
    
    net_endpoint_monitor_t monitor = TAILQ_FIRST(&endpoint->m_monitors);
    while(monitor && endpoint->m_state != net_endpoint_state_deleting) {
        net_endpoint_monitor_t next_monitor = TAILQ_NEXT(monitor, m_next);
        if (monitor->m_is_free) {
            monitor = next_monitor;
            continue;
        }

        if (monitor->m_on_state_change) {
            uint8_t tag_local = 0;
            if (!monitor->m_is_processing) {
                tag_local = 1;
                monitor->m_is_processing = 1;
            }
            monitor->m_on_state_change(monitor->m_ctx, endpoint, old_state);
            if (tag_local) {
                monitor->m_is_processing = 0;
                if (monitor->m_is_free) net_endpoint_monitor_free(monitor);
            }
        }
        
        monitor = next_monitor;
    }

    return rv;
}
