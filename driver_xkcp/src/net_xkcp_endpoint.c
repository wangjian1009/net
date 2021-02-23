#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_dgram.h"
#include "net_timer.h"
#include "net_xkcp_endpoint_i.h"
#include "net_xkcp_connector_i.h"
#include "net_xkcp_client_i.h"
#include "net_xkcp_acceptor_i.h"
#include "net_xkcp_config_i.h"
#include "net_xkcp_utils.h"

static int net_xkcp_endpoint_kcp_output(const char *buf, int len, ikcpcb *xkcp, void *user);
static void net_xkcp_endpoint_kcp_do_update(net_timer_t timer, void * ctx);
static void net_xkcp_endpoint_on_write(net_xkcp_endpoint_t endpoint);
static void net_xkcp_endpoint_apply_config(net_xkcp_endpoint_t endpoint, net_xkcp_config_t config);
    
int net_xkcp_endpoint_init(net_endpoint_t base_endpoint) {
    net_xkcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_conv = 0;
    endpoint->m_kcp = NULL;
    endpoint->m_runing_mode = net_xkcp_endpoint_runing_mode_init;

    endpoint->m_kcp_update_timer =
        net_timer_auto_create(
            net_endpoint_schedule(base_endpoint), net_xkcp_endpoint_kcp_do_update, base_endpoint);
    if (endpoint->m_kcp_update_timer == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: init: create kcp update timer fail!",
            net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }
    
    return 0;
}

void net_xkcp_endpoint_fini(net_endpoint_t base_endpoint) {
    net_xkcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_kcp) {
        ikcp_flush(endpoint->m_kcp);
    }

    net_xkcp_endpoint_set_runing_mode(endpoint, net_xkcp_endpoint_runing_mode_init);

    if (endpoint->m_kcp_update_timer) {
        net_timer_free(endpoint->m_kcp_update_timer);
        endpoint->m_kcp_update_timer = NULL;
    }
}

int net_xkcp_endpoint_connect(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_xkcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    net_address_t target_addr = net_endpoint_remote_address(base_endpoint);
    if (target_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: connect: target addr not set!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    switch(endpoint->m_runing_mode) {
    case net_xkcp_endpoint_runing_mode_init:
        net_xkcp_endpoint_set_runing_mode(endpoint, net_xkcp_endpoint_runing_mode_cli);
        break;
    case net_xkcp_endpoint_runing_mode_cli:
        break;
    case net_xkcp_endpoint_runing_mode_svr:
        CPE_ERROR(
            driver->m_em, "xkcp: %s: connect: can`t connect in runing-mode %s!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_xkcp_endpoint_runing_mode_str(endpoint->m_runing_mode));
        return -1;
    }

    if (endpoint->m_conv != 0) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: connect: already connected, conv=%d",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            endpoint->m_conv);
        return -1;
    }
    
    if (endpoint->m_cli.m_connector == NULL) {
        net_xkcp_connector_t connector = net_xkcp_connector_find(driver, target_addr);
        if (connector == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s: connect: no connector to target!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
            return -1;
        }

        if (net_xkcp_endpoint_set_connector(endpoint, connector) != 0) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s: connect: set connector fail!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
            return -1;
        }
    }

    if (net_xkcp_endpoint_set_conv(endpoint, ++endpoint->m_cli.m_connector->m_max_conv) != 0) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: connect: set conv fail!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    return net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_established);
}

int net_xkcp_endpoint_update(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_xkcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_read_closed:
        return 0;
    case net_endpoint_state_write_closed:
        return 0;
    case net_endpoint_state_disable:
        if (endpoint->m_kcp) {
            ikcp_flush(endpoint->m_kcp);
        }
        if (endpoint->m_conv) {
            net_xkcp_endpoint_set_conv(endpoint, 0);
        }
        return 0;
    case net_endpoint_state_established:
        if (!net_endpoint_is_writing(endpoint->m_base_endpoint)) {
            net_xkcp_endpoint_on_write(endpoint);
        }
        return 0;
    case net_endpoint_state_error:
        return 0;
    default:
        return 0;
    }
}

int net_xkcp_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_xkcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    return 0;
}

int net_xkcp_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_xkcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_kcp == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: get : no bind kcp!",
            net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    *mss = endpoint->m_kcp->mss;
    return 0;
}

net_xkcp_endpoint_t
net_xkcp_endpoint_create(net_xkcp_driver_t driver, net_protocol_t protocol) {
    net_driver_t base_driver = net_driver_from_data(driver);
    
    net_endpoint_t base_endpoint = net_endpoint_create(base_driver, protocol, NULL);
    if (base_endpoint == NULL) return NULL;

    return net_endpoint_data(base_endpoint);
}

net_xkcp_endpoint_t
net_xkcp_endpoint_cast(net_endpoint_t base_endpoint) {
    net_driver_t driver = net_endpoint_driver(base_endpoint);
    return net_driver_endpoint_init_fun(driver) == net_xkcp_endpoint_init
        ? net_endpoint_data(base_endpoint)
        : NULL;
}

net_endpoint_t
net_xkcp_endpoint_base_endpoint(net_xkcp_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

uint32_t net_xkcp_endpoint_conv(net_xkcp_endpoint_t endpoint) {
    return endpoint->m_conv;
}

net_xkcp_endpoint_runing_mode_t net_xkcp_endpoint_runing_mode(net_xkcp_endpoint_t endpoint) {
    return endpoint->m_runing_mode;
}

void net_xkcp_endpoint_set_runing_mode(net_xkcp_endpoint_t endpoint, net_xkcp_endpoint_runing_mode_t runing_mode) {
    if (endpoint->m_runing_mode == runing_mode) return;

    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(endpoint->m_base_endpoint));
    
    if (net_endpoint_driver_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            driver->m_em, "xkcp: %s: runint-mode %s ==> %s",
            net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint),
            net_xkcp_endpoint_runing_mode_str(endpoint->m_runing_mode),
            net_xkcp_endpoint_runing_mode_str(runing_mode));
    }

    switch(endpoint->m_runing_mode) {
    case net_xkcp_endpoint_runing_mode_init:
        break;
    case net_xkcp_endpoint_runing_mode_cli:
        net_xkcp_endpoint_set_connector(endpoint, NULL);
        net_xkcp_endpoint_set_conv(endpoint, 0);
        break;
    case net_xkcp_endpoint_runing_mode_svr:
        net_xkcp_endpoint_set_client(endpoint, NULL);
        net_xkcp_endpoint_set_conv(endpoint, 0);
        break;
    }

    endpoint->m_runing_mode = runing_mode;

    switch(endpoint->m_runing_mode) {
    case net_xkcp_endpoint_runing_mode_init:
        break;
    case net_xkcp_endpoint_runing_mode_cli:
        endpoint->m_cli.m_connector = NULL;
        break;
    case net_xkcp_endpoint_runing_mode_svr:
        endpoint->m_svr.m_client = NULL;
        break;
    }
}

int net_xkcp_endpoint_set_conv(net_xkcp_endpoint_t endpoint, uint32_t conv) {
    net_endpoint_t base_endpoint = endpoint->m_base_endpoint;
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    if (endpoint->m_conv) {
        switch(endpoint->m_runing_mode) {
        case net_xkcp_endpoint_runing_mode_init:
            break;
        case net_xkcp_endpoint_runing_mode_cli:
            if (endpoint->m_cli.m_connector) {
                cpe_hash_table_remove_by_ins(&endpoint->m_cli.m_connector->m_streams, endpoint);
            }
            break;
        case net_xkcp_endpoint_runing_mode_svr:
            if (endpoint->m_svr.m_client) {
                cpe_hash_table_remove_by_ins(&endpoint->m_svr.m_client->m_streams, endpoint);
            }
            break;
        }

        if (endpoint->m_kcp) {
            ikcp_release(endpoint->m_kcp);
            endpoint->m_kcp = NULL;
        }
    }

    assert(endpoint->m_kcp == NULL);
    endpoint->m_conv = conv;

    if (endpoint->m_conv) {
        endpoint->m_kcp = ikcp_create(endpoint->m_conv, endpoint);
        if (endpoint->m_kcp == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s: set conv: create kcp fail",
                net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }
        ikcp_setoutput(endpoint->m_kcp, net_xkcp_endpoint_kcp_output);

        switch(endpoint->m_runing_mode) {
        case net_xkcp_endpoint_runing_mode_init:
            break;
        case net_xkcp_endpoint_runing_mode_cli:
            if (endpoint->m_cli.m_connector) {
                net_xkcp_config_t config = endpoint->m_cli.m_connector->m_config;
                if (config) {
                    net_xkcp_endpoint_apply_config(endpoint, config);
                }
                
                cpe_hash_entry_init(&endpoint->m_cli.m_hh_for_connector);
                if (cpe_hash_table_insert_unique(&endpoint->m_cli.m_connector->m_streams, endpoint) != 0) {
                    CPE_ERROR(
                        driver->m_em, "xkcp: %s: set conv: conv %d duplicate",
                        net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint),
                        endpoint->m_kcp->conv);
                    ikcp_release(endpoint->m_kcp);
                    endpoint->m_kcp = NULL;
                    return -1;
                }
            }
            break;
        case net_xkcp_endpoint_runing_mode_svr:
            if (endpoint->m_svr.m_client) {
                net_xkcp_config_t config = endpoint->m_svr.m_client->m_acceptor->m_config;
                if (config) {
                    net_xkcp_endpoint_apply_config(endpoint, config);
                }
                
                cpe_hash_entry_init(&endpoint->m_svr.m_hh_for_client);
                if (cpe_hash_table_insert_unique(&endpoint->m_svr.m_client->m_streams, endpoint) != 0) {
                    CPE_ERROR(
                        driver->m_em, "xkcp: %s: set conv: conv %d duplicate",
                        net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint),
                        endpoint->m_kcp->conv);
                    ikcp_release(endpoint->m_kcp);
                    endpoint->m_kcp = NULL;
                    return -1;
                }
            }
            break;
        }
    }
    
    return 0;
}

int net_xkcp_endpoint_set_connector(net_xkcp_endpoint_t endpoint, net_xkcp_connector_t connector) {
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(endpoint->m_base_endpoint));
    
    if (endpoint->m_runing_mode != net_xkcp_endpoint_runing_mode_cli) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: set connector: can`t set connector in runing-mode %s",
            net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint),
            net_xkcp_endpoint_runing_mode_str(endpoint->m_runing_mode));
        return -1;
    }

    if (endpoint->m_cli.m_connector == connector) return 0;

    if (endpoint->m_cli.m_connector) {
        if (endpoint->m_conv) {
            cpe_hash_table_remove_by_ins(&endpoint->m_cli.m_connector->m_streams, endpoint);
        }
        endpoint->m_cli.m_connector = NULL;
    }

    endpoint->m_cli.m_connector = connector;

    if (endpoint->m_cli.m_connector) {
        if (endpoint->m_conv) {
            cpe_hash_entry_init(&endpoint->m_cli.m_hh_for_connector);
            if (cpe_hash_table_insert_unique(&endpoint->m_cli.m_connector->m_streams, endpoint) != 0) {
                CPE_ERROR(
                    driver->m_em, "xkcp: %s: set connector: conv %d duplicate",
                    net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint),
                    endpoint->m_kcp->conv);
                endpoint->m_cli.m_connector = NULL;
                return -1;
            }
        }

        if (endpoint->m_kcp) {
            net_xkcp_config_t config = endpoint->m_cli.m_connector->m_config;
            if (config) {
                net_xkcp_endpoint_apply_config(endpoint, config);
            }
        }
    }

    return 0;
}

int net_xkcp_endpoint_set_client(net_xkcp_endpoint_t endpoint, net_xkcp_client_t client) {
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(endpoint->m_base_endpoint));
    
    if (endpoint->m_runing_mode != net_xkcp_endpoint_runing_mode_svr) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: set client: can`t set client in runing-mode %s",
            net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint),
            net_xkcp_endpoint_runing_mode_str(endpoint->m_runing_mode));
        return -1;
    }

    if (endpoint->m_svr.m_client == client) return 0;

    if (endpoint->m_svr.m_client) {
        if (endpoint->m_conv) {
            cpe_hash_table_remove_by_ins(&endpoint->m_svr.m_client->m_streams, endpoint);
        }
        endpoint->m_svr.m_client = NULL;
    }

    endpoint->m_svr.m_client = client;

    if (endpoint->m_svr.m_client) {
        if (endpoint->m_conv) {
            cpe_hash_entry_init(&endpoint->m_svr.m_hh_for_client);
            if (cpe_hash_table_insert_unique(&endpoint->m_svr.m_client->m_streams, endpoint) != 0) {
                CPE_ERROR(
                    driver->m_em, "xkcp: %s: set client: conv %d duplicate",
                    net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint),
                    endpoint->m_conv);
                endpoint->m_svr.m_client = NULL;
                return -1;
            }

            if (endpoint->m_kcp) {
                net_xkcp_config_t config = endpoint->m_svr.m_client->m_acceptor->m_config;
                if (config) {
                    net_xkcp_endpoint_apply_config(endpoint, config);
                }
            }
        }
    }

    return 0;
}

int net_xkcp_endpoint_kcp_output(const char *buf, int len, ikcpcb *xkcp, void *user) {
    net_xkcp_endpoint_t endpoint = user;
    net_endpoint_t base_endpoint = endpoint->m_base_endpoint;
    net_driver_t base_driver = net_endpoint_driver(base_endpoint);
    net_xkcp_driver_t driver = net_driver_data(base_driver);

    net_address_t remote_address = NULL;
    net_dgram_t dgram = NULL;

    switch(endpoint->m_runing_mode) {
    case net_xkcp_endpoint_runing_mode_init:
        CPE_ERROR(
            driver->m_em, "xkcp: %s: kcp output: can`t output in runing-mode %s",
            net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint),
            net_xkcp_endpoint_runing_mode_str(endpoint->m_runing_mode));
        return -1;
    case net_xkcp_endpoint_runing_mode_cli:
        if (endpoint->m_cli.m_connector == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s: kcp output: no binding connector",
                net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint));
            return -1;
        }
        remote_address = endpoint->m_cli.m_connector->m_remote_address;
        dgram = endpoint->m_cli.m_connector->m_dgram;
        break;
    case net_xkcp_endpoint_runing_mode_svr:
        if (endpoint->m_svr.m_client == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s: kcp output: no binding client",
                net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint));
            return -1;
        }
        remote_address = endpoint->m_svr.m_client->m_remote_address;
        dgram = endpoint->m_svr.m_client->m_acceptor->m_dgram;
        break;
    }

    if (net_dgram_send(dgram, remote_address, buf, len) != 0) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: kcp output: dgram send error",
            net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), endpoint->m_base_endpoint));
        return -1;
    }

    if (net_driver_debug(base_driver) >= 2) {
        CPE_INFO(
            driver->m_em, "xkcp: %s",
            net_xkcp_dump_frame_with_address(
                net_xkcp_driver_tmp_buffer(driver),
                net_dgram_address(dgram), remote_address, 1, buf, len));
    }

    return 0;
}

void net_xkcp_endpoint_kcp_schedule_update(net_xkcp_endpoint_t endpoint) {
    if (endpoint->m_kcp == NULL) {
        net_timer_cancel(endpoint->m_kcp_update_timer);
    }
    else {
        IUINT32 cur_time_ms = (IUINT32 )net_schedule_cur_time_ms(net_endpoint_schedule(endpoint->m_base_endpoint));
        IUINT32 next_time_ms = ikcp_check(endpoint->m_kcp, cur_time_ms);
        assert(next_time_ms >= cur_time_ms);
        net_timer_active(endpoint->m_kcp_update_timer, next_time_ms - cur_time_ms);
    }
}

void net_xkcp_endpoint_on_write(net_xkcp_endpoint_t endpoint) {
    net_driver_t base_driver = net_endpoint_driver(endpoint->m_base_endpoint);
    net_xkcp_driver_t driver = net_driver_data(base_driver);
    net_endpoint_t base_endpoint = endpoint->m_base_endpoint;
    uint8_t need_schedule = 0;

    while(
        net_endpoint_is_writeable(base_endpoint)
        && !net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)
        && !net_endpoint_is_writing(base_endpoint)
        && endpoint->m_kcp)
    {
        uint32_t data_size = 0;
        void * data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_write, &data_size);
        assert(data_size > 0);
        assert(data);

        int rv = ikcp_send(endpoint->m_kcp, data, data_size);
        if (rv < 0) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s: conv=%d: send error, errno=%d!",
                net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint), endpoint->m_conv,
                rv);

            if (net_endpoint_error_source(base_endpoint) == net_endpoint_error_source_none) {
                net_endpoint_set_error(
                    base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error, NULL);
            }
            
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            }
            else {
                if (net_endpoint_is_writing(base_endpoint)) {
                    net_endpoint_set_is_writing(base_endpoint, 0);
                }
            }

            return;
        }

        need_schedule = 1;
        if (net_endpoint_driver_debug(base_endpoint)) {
            CPE_INFO(
                driver->m_em, "xkcp: %s: conv %d: send %d bytes data!",
                net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint),
                endpoint->m_conv,
                data_size);
        }

        net_endpoint_buf_consume(base_endpoint, net_ep_buf_write, data_size);
    }

    if (need_schedule && net_endpoint_is_active(base_endpoint) && endpoint->m_kcp) {
        net_xkcp_endpoint_kcp_schedule_update(endpoint);
    }
}

void net_xkcp_endpoint_kcp_forward_data(net_xkcp_endpoint_t endpoint) {
    net_driver_t base_driver = net_endpoint_driver(endpoint->m_base_endpoint);
    net_xkcp_driver_t driver = net_driver_data(base_driver);
    net_endpoint_t base_endpoint = endpoint->m_base_endpoint;
    
	while(net_endpoint_is_readable(base_endpoint)
          && net_endpoint_expect_read(base_endpoint)
          && endpoint->m_kcp)
    {
        int ikcp_size = ikcp_peeksize(endpoint->m_kcp);
        if (ikcp_size <= 0) break;

        uint32_t capacity = ikcp_size;

        void * buf = net_endpoint_buf_alloc_suggest(base_endpoint, &capacity);
        if (buf == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s: alloc input buf fail, capacity=%d",
                net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint), capacity);

            if (net_endpoint_error_source(base_endpoint) == net_endpoint_error_source_none) {
                net_endpoint_set_error(
                    base_endpoint,
                    net_endpoint_error_source_network, net_endpoint_network_errno_logic, "alloc buf fail!");
            }
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            }
            return;
        }

        int nrecv = ikcp_recv(endpoint->m_kcp, buf, capacity);
		if (nrecv < 0) {
            net_endpoint_buf_release(base_endpoint);

            CPE_ERROR(
                driver->m_em, "xkcp: %s: conv=%d: recv error, errno=%d!",
                net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint), endpoint->m_conv,
                nrecv);

            if (net_endpoint_error_no(base_endpoint) == 0) {
                net_endpoint_set_error(
                    base_endpoint, net_endpoint_error_source_network,
                    net_endpoint_network_errno_logic, NULL);
            }

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            }

            return;
		}

        if (net_endpoint_driver_debug(base_endpoint)) {
            CPE_INFO(
                driver->m_em, "xkcp: %s: conv=%d: recv %d bytes data!",
                net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint), endpoint->m_conv,
                (int)nrecv);
        }
        
        if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_read, nrecv) != 0) {
            if (net_endpoint_is_active(base_endpoint)) {
                if (!net_endpoint_have_error(base_endpoint)) {
                    net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic, NULL);
                }

                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) {
                    net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
                }
            }
            return;
        }
	}
}

static void net_xkcp_endpoint_kcp_do_update(net_timer_t timer, void * ctx) {
    net_endpoint_t base_endpoint = ctx;
    net_xkcp_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_kcp) {
        IUINT32 cur_time_ms = (IUINT32 )net_schedule_cur_time_ms(net_endpoint_schedule(base_endpoint));
        ikcp_update(endpoint->m_kcp, cur_time_ms);

        if (net_endpoint_is_active(base_endpoint) && endpoint->m_kcp) {
            IUINT32 next_time_ms = ikcp_check(endpoint->m_kcp, cur_time_ms);
            if (next_time_ms > cur_time_ms) {
                net_timer_active(timer, next_time_ms - cur_time_ms);
            }
        }
    }
}

static void net_xkcp_endpoint_apply_config(net_xkcp_endpoint_t endpoint, net_xkcp_config_t config) {
    net_endpoint_t base_endpoint = endpoint->m_base_endpoint;
    net_xkcp_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    assert(endpoint->m_kcp);
    assert(config);
    
    if (net_endpoint_driver_debug(base_endpoint)) {
        char buf[128];
        cpe_str_dup(
            buf, sizeof(buf),
            net_endpoint_dump(net_xkcp_driver_tmp_buffer(driver), base_endpoint));
        
        CPE_INFO(
            driver->m_em, "xkcp: %s: conv=%d: apply config %s!",
            buf, endpoint->m_conv,
            net_xkcp_config_dump(net_xkcp_driver_tmp_buffer(driver), config));
    }

    ikcp_setmtu(endpoint->m_kcp, config->m_mtu);
	ikcp_wndsize(endpoint->m_kcp, config->m_send_wnd, config->m_recv_wnd);
    
    switch(config->m_mode) {
    case net_xkcp_mode_normal:
        ikcp_nodelay(endpoint->m_kcp, 0, 40, 2, 1);
        break;
    case net_xkcp_mode_fast:
        ikcp_nodelay(endpoint->m_kcp, 0, 30, 2, 1);
        break;
    case net_xkcp_mode_fast2:
        ikcp_nodelay(endpoint->m_kcp, 1, 20, 2, 1);
        break;
    case net_xkcp_mode_fast3:
        ikcp_nodelay(endpoint->m_kcp, 1, 10, 2, 1);
        break;
    }
}

int net_xkcp_endpoint_eq(net_xkcp_endpoint_t l, net_xkcp_endpoint_t r, void * user_data) {
    return l->m_conv == r->m_conv ? 1 : 0;
}

uint32_t net_xkcp_endpoint_hash(net_xkcp_endpoint_t o, void * user_data) {
    return o->m_conv;
}

const char * net_xkcp_endpoint_runing_mode_str(net_xkcp_endpoint_runing_mode_t runing_mode) {
    switch(runing_mode) {
    case net_xkcp_endpoint_runing_mode_init:
        return "init";
    case net_xkcp_endpoint_runing_mode_cli:
        return "cli";
    case net_xkcp_endpoint_runing_mode_svr:
        return "svr";
    }
}
