#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint_i.h"
#include "net_mem_block_i.h"
#include "net_endpoint_monitor_i.h"
#include "net_debug_setup_i.h"
#include "net_protocol_i.h"
#include "net_driver_i.h"
#include "net_schedule_i.h"
#include "net_address_i.h"
#include "net_link_i.h"
#include "net_dns_query_i.h"
#include "net_endpoint_next_i.h"

static void net_endpoint_dns_query_callback(void * ctx, net_address_t main_address, net_address_it_t it);
static int net_endpoint_notify_state_changed(net_endpoint_t endpoint, net_endpoint_state_t old_state);

net_endpoint_t
net_endpoint_create(net_driver_t driver, net_protocol_t protocol, net_mem_group_t mem_group) {
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
    endpoint->m_mem_group = mem_group ? mem_group : schedule->m_dft_mem_group;
    endpoint->m_prepare_connect = NULL;
    endpoint->m_prepare_connect_ctx = NULL;
    endpoint->m_close_after_send = 0;
    endpoint->m_protocol_debug = protocol->m_debug;
    endpoint->m_driver_debug = driver->m_debug;
    endpoint->m_error_source = net_endpoint_error_source_network;
    endpoint->m_error_no = 0;
    endpoint->m_error_msg = NULL;
    endpoint->m_link = NULL;
    endpoint->m_id = schedule->m_endpoint_max_id + 1;
    endpoint->m_dft_block_size = 0;
    endpoint->m_options = 0;
    endpoint->m_expect_read = 1;
    endpoint->m_is_writing = 0;
    endpoint->m_state = net_endpoint_state_disable;
    endpoint->m_dns_query = NULL;
    endpoint->m_tb = NULL;

    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
        TAILQ_INIT(&endpoint->m_bufs[i].m_blocks);
        endpoint->m_bufs[i].m_size = 0;
    }
    
    endpoint->m_data_watcher_ctx = NULL;
    endpoint->m_data_watcher_fun = NULL;
    endpoint->m_data_watcher_fini = NULL;

    TAILQ_INIT(&endpoint->m_monitors);
    TAILQ_INIT(&endpoint->m_nexts);

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
    TAILQ_INSERT_TAIL(&protocol->m_endpoints, endpoint, m_next_for_protocol);

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

    TAILQ_REMOVE(&endpoint->m_protocol->m_endpoints, endpoint, m_next_for_protocol);
    
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
        net_mem_block_free(endpoint->m_tb);
        endpoint->m_tb = NULL;
    }
    
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
        while(!TAILQ_EMPTY(&endpoint->m_bufs[i].m_blocks)) {
            net_mem_block_free(TAILQ_FIRST(&endpoint->m_bufs[i].m_blocks));
        }
        endpoint->m_bufs[i].m_size = 0;
    }

    while(!TAILQ_EMPTY(&endpoint->m_monitors)) {
        net_endpoint_monitor_t monitor = TAILQ_FIRST(&endpoint->m_monitors);
        assert(!monitor->m_is_processing);
        assert(!monitor->m_is_free);
        net_endpoint_monitor_free(monitor);
    }

    while(!TAILQ_EMPTY(&endpoint->m_nexts)) {
        net_endpoint_next_free(TAILQ_FIRST(&endpoint->m_nexts));
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

    TAILQ_REMOVE(&old_protocol->m_endpoints, endpoint, m_next_for_protocol);
    TAILQ_INSERT_TAIL(&protocol->m_endpoints, endpoint, m_next_for_protocol);
    
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

const char * net_endpoint_driver_name(net_endpoint_t endpoint) {
    return endpoint->m_driver->m_name;
}

uint32_t net_endpoint_id(net_endpoint_t endpoint) {
    return endpoint->m_id;
}

net_endpoint_t net_endpoint_find(net_schedule_t schedule, uint32_t id) {
    struct net_endpoint key;
    key.m_id = id;
    return cpe_hash_table_find(&schedule->m_endpoints, &key);
}

uint32_t net_endpoint_dft_block_size(net_endpoint_t endpoint) {
    return endpoint->m_dft_block_size;
}

void net_endpoint_set_dft_block_size(net_endpoint_t endpoint, uint32_t block_size) {
    endpoint->m_dft_block_size = block_size;
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
            while(!TAILQ_EMPTY(&endpoint->m_bufs[i].m_blocks)) {
                net_mem_block_free(TAILQ_FIRST(&endpoint->m_bufs[i].m_blocks));
            }
            endpoint->m_bufs[i].m_size = 0;
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

uint8_t net_endpoint_expect_read(net_endpoint_t endpoint) {
    return endpoint->m_expect_read;
}

void net_endpoint_set_expect_read(net_endpoint_t endpoint, uint8_t expect_read) {
    if (endpoint->m_expect_read == expect_read) return;

    endpoint->m_expect_read= expect_read;

    if (endpoint->m_driver->m_endpoint_update) {
        endpoint->m_driver->m_endpoint_update(endpoint);
    }
}

uint8_t net_endpoint_is_writing(net_endpoint_t endpoint) {
    return endpoint->m_is_writing;
}

void net_endpoint_set_is_writing(net_endpoint_t endpoint, uint8_t is_writing) {
    is_writing = is_writing ? 1 : 0;
    
    if (endpoint->m_is_writing == is_writing) return;

    endpoint->m_is_writing = is_writing;

    struct net_endpoint_monitor_evt evt;
    evt.m_type = is_writing ? net_endpoint_monitor_evt_write_begin : net_endpoint_monitor_evt_write_completed;
    net_endpoint_send_evt(endpoint, &evt);
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

    while(!TAILQ_EMPTY(&endpoint->m_nexts)) {
        net_endpoint_next_free(TAILQ_FIRST(&endpoint->m_nexts));
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

        net_dns_query_type_t query_type;
        switch(schedule->m_local_ip_stack) {
        case net_local_ip_stack_none:
        case net_local_ip_stack_dual:
            query_type = net_dns_query_ipv4v6;
            break;
        case net_local_ip_stack_ipv4:
            query_type = net_dns_query_ipv4;
            break;
        case net_local_ip_stack_ipv6:
            query_type = net_dns_query_ipv6;
            break;
        }

        endpoint->m_dns_query =
            net_dns_query_create(
                schedule, endpoint->m_remote_address, query_type,
                NULL,
                net_endpoint_dns_query_callback, NULL, endpoint);
        if (endpoint->m_dns_query == NULL) {
            CPE_ERROR(
                schedule->m_em, "%s: connect: create dns query fail!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
            return -1;
        }

        if (net_endpoint_set_state(endpoint, net_endpoint_state_resolving) != 0) return -1;

        if (endpoint->m_driver_debug || schedule->m_debug >= 2) {
            CPE_INFO(
                schedule->m_em, "%s: resolve: begin",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        }
        
        return 0;
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
        net_endpoint_set_error(
            endpoint, net_endpoint_error_source_network,
            net_endpoint_network_errno_dns_error, "dns resolve error");
        if (net_endpoint_set_state(endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_free(endpoint);
        }
        return;
    }

    assert(endpoint->m_remote_address);
    if (net_address_set_resolved(endpoint->m_remote_address, address, 0) != 0) {
        CPE_ERROR(
            schedule->m_em, "%s: resolve: set resolve result fail!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        if (net_endpoint_set_state(endpoint, net_endpoint_state_logic_error) != 0) {
            net_endpoint_free(endpoint);
        }
        return;
    }

    uint32_t other_address_count = 0;
    net_address_t other_address;
    while((other_address = net_address_it_next(all_address))) {
        if (net_address_type(other_address) != net_address_ipv4
            && net_address_type(other_address) != net_address_ipv6) continue;
        if (net_address_cmp_without_port(address, other_address) == 0) continue;

        net_endpoint_next_t next = net_endpoint_next_create(endpoint, other_address);
        if (next == NULL) {
            CPE_ERROR(
                schedule->m_em, "%s: resolve: resolve success, add next try address fail!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
            continue;
        }

        other_address_count++;
    }
    
    if (endpoint->m_driver_debug || schedule->m_debug >= 2) {
        char address_buf[128];
        cpe_str_dup(address_buf, sizeof(address_buf), net_address_host(&schedule->m_tmp_buffer, address));
        
        CPE_INFO(
            schedule->m_em, "%s: resolve: success, use-address=%s, next-try-address=%d!",
            net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
            address_buf,
            other_address_count);
    }
    
    while (endpoint->m_driver->m_endpoint_connect(endpoint) != 0) {
        if (net_endpoint_shift_address(endpoint)) { /*有下一个地址，尝试连接下一个地址 */
            continue;
        }
        else {
            if (endpoint->m_state != net_endpoint_state_deleting) {
                if (net_endpoint_set_state(endpoint, net_endpoint_state_network_error) != 0) {
                    net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
                }
            }
            return;
        }
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

uint8_t net_endpoint_shift_address(net_endpoint_t endpoint) {
    assert(endpoint->m_state != net_endpoint_state_deleting);

    if (endpoint->m_remote_address == NULL) return 0;

    net_endpoint_next_t next;

    while((next = TAILQ_FIRST(&endpoint->m_nexts))) {
        net_address_t next_address = next->m_address;
        assert(next_address);
        next->m_address = NULL;
        net_endpoint_next_free(next);

        net_schedule_t schedule = endpoint->m_driver->m_schedule;

        if (endpoint->m_driver_debug || schedule->m_debug >= 2) {
            char address_buf[128];
            cpe_str_dup(address_buf, sizeof(address_buf), net_address_host(&schedule->m_tmp_buffer, next_address));
    
            CPE_INFO(
                schedule->m_em, "%s: shift address: use %s",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint), address_buf);
        }

        if (net_address_set_resolved(endpoint->m_remote_address, next_address, 1) != 0) {
            CPE_ERROR(
                schedule->m_em, "%s: shift address: set resolve result fail!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
            net_address_free(next_address);
            continue;
        }

        return 1;
    }

    return 0;
}

static int net_endpoint_notify_state_changed(net_endpoint_t endpoint, net_endpoint_state_t old_state) {
    int rv = endpoint->m_protocol->m_endpoint_on_state_chagne
        ? endpoint->m_protocol->m_endpoint_on_state_chagne(endpoint, old_state)
        : net_endpoint_is_active(endpoint)
        ? 0
        : -1;
    
    struct net_endpoint_monitor_evt evt;
    evt.m_type = net_endpoint_monitor_evt_state_changed;
    evt.m_from_state = old_state;
    net_endpoint_send_evt(endpoint, &evt);
    
    return rv;
}

void net_endpoint_send_evt(net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt) {
    net_endpoint_monitor_t monitor = TAILQ_FIRST(&endpoint->m_monitors);
    while(monitor && endpoint->m_state != net_endpoint_state_deleting) {
        net_endpoint_monitor_t next_monitor = TAILQ_NEXT(monitor, m_next);
        if (monitor->m_is_free) {
            monitor = next_monitor;
            continue;
        }

        if (monitor->m_cb) {
            uint8_t tag_local = 0;
            if (!monitor->m_is_processing) {
                tag_local = 1;
                monitor->m_is_processing = 1;
            }

            monitor->m_cb(monitor->m_ctx, endpoint, evt);

            if (tag_local) {
                monitor->m_is_processing = 0;
                if (monitor->m_is_free) net_endpoint_monitor_free(monitor);
            }
        }
        
        monitor = next_monitor;
    }
}
