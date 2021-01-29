#include <assert.h>
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ws_svr_underline_i.h"

#define net_ws_svr_underline_ep_write_cache net_ep_buf_user1
#define net_ws_svr_underline_ep_read_cache net_ep_buf_user2

static void net_ws_svr_underline_dump_error(net_endpoint_t base_underline, int val);

static int net_ws_svr_underline_do_handshake(net_endpoint_t base_underline, net_ws_svr_underline_t underline);

static int net_ws_svr_underline_update_error(net_endpoint_t base_underline, int err, int r);

int net_ws_svr_underline_init(net_endpoint_t base_underline) {
    net_ws_svr_underline_t underline = net_endpoint_protocol_data(base_underline);
    net_ws_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ws_svr_driver_t driver = protocol->m_driver;
    net_driver_t base_driver = net_driver_from_data(driver);

    underline->m_ws_endpoint = NULL;
    underline->m_state = net_ws_svr_underline_ws_handshake;

    return 0;
}

void net_ws_svr_underline_fini(net_endpoint_t base_underline) {
    net_ws_svr_underline_t underline = net_endpoint_protocol_data(base_underline);

    if (underline->m_ws_endpoint) {
        assert(underline->m_ws_endpoint->m_underline == base_underline);
        underline->m_ws_endpoint->m_underline = NULL;
        underline->m_ws_endpoint = NULL;
    }
}

int net_ws_svr_underline_input(net_endpoint_t base_underline) {
    net_ws_svr_underline_t underline = net_endpoint_protocol_data(base_underline);
    net_ws_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ws_svr_driver_t driver = protocol->m_driver;

    if (underline->m_state == net_ws_svr_underline_ws_handshake) {
        if (net_ws_svr_underline_do_handshake(base_underline, underline) != 0) return -1;
    }

READ_AGAIN:    
    if (net_endpoint_state(base_underline) != net_endpoint_state_established) return -1;

    /* uint32_t input_data_size = net_endpoint_buf_size(base_underline, net_ep_buf_read); */
    /* if (input_data_size == 0) return 0; */

    /* void * data = net_endpoint_buf_alloc_at_least(base_underline, &input_data_size); */
    /* if (data == NULL) { */
    /*     CPE_ERROR( */
    /*         driver->m_em, "net: ws: %s: input: alloc data for read fail", */
    /*         net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline)); */
    /*     return -1; */
    /* } */

    /* ERR_clear_error(); */
    /* int r = SSL_read(underline->m_ws, data, input_data_size); */
    /* if (r > 0) { */
    /*     if (net_endpoint_buf_supply(base_underline, net_ws_svr_underline_ep_read_cache, r) != 0) { */
    /*         CPE_ERROR( */
    /*             driver->m_em, "net: ws: %s: input: load data to input cache fail", */
    /*             net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline)); */
    /*         return -1; */
    /*     } */

    /*     net_endpoint_t base_endpoint = */
    /*         underline->m_ws_endpoint ? net_endpoint_from_data(underline->m_ws_endpoint) : NULL; */

    /*     if (base_endpoint */
    /*         && net_endpoint_state(base_endpoint) == net_endpoint_state_established) */
    /*     { */
    /*         if (net_endpoint_buf_append_from_other( */
    /*                 base_endpoint, net_ep_buf_read, */
    /*                 base_underline, net_ws_svr_underline_ep_read_cache, 0) != 0) */
    /*         { */
    /*             CPE_ERROR( */
    /*                 driver->m_em, "net: ws: %s: input: forward data to ws ep fail", */
    /*                 net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline)); */
    /*             return -1; */
    /*         } */
    /*         else { */
    /*             goto READ_AGAIN; */
    /*         } */
    /*     } */
    /*     else { */
    /*         net_endpoint_buf_clear(base_underline, net_ws_svr_underline_ep_read_cache); */
    /*         CPE_ERROR( */
    /*             driver->m_em, "net: ws: %s: input: no established ws endpoint(state=%s), clear cached data", */
    /*             net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline), */
    /*             net_endpoint_state_str(net_endpoint_state(base_endpoint))); */
    /*     } */
    /* } */
    /* else { */
    /*     net_endpoint_buf_release(base_underline); */

    /*     int err = SSL_get_error(underline->m_ws, r); */
    /*     return net_ws_svr_underline_update_error(base_underline, err, r); */
    /* } */
    
    return 0;
}

int net_ws_svr_underline_on_state_change(net_endpoint_t base_underline, net_endpoint_state_t from_state) {
    net_schedule_t schedule = net_endpoint_schedule(base_underline);

    switch(net_endpoint_state(base_underline)) {
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        CPE_ERROR(
            net_schedule_em(schedule),
            "net: ws: %s: state update: not support state %s",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_underline),
            net_endpoint_state_str(net_endpoint_state(base_underline)));
        assert(0);
        return 0;
    default:
        break;
    }
        
    net_ws_svr_underline_t underline = net_endpoint_protocol_data(base_underline);

    if (underline->m_ws_endpoint == NULL) {
        switch (net_endpoint_state(base_underline)) {
        case net_endpoint_state_resolving:
        case net_endpoint_state_connecting:
        case net_endpoint_state_deleting:
            assert(0);
            return 0;
        case net_endpoint_state_established:
            /*在acceptor的情况下，undliner提前进入establish，此处保护*/
            return 0;
        case net_endpoint_state_error:
        case net_endpoint_state_disable:
        case net_endpoint_state_read_closed:
        case net_endpoint_state_write_closed:
            CPE_ERROR(
                net_schedule_em(schedule),
                "net: ws: %s: state updated: no ws endpoint %s",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_underline),
                net_endpoint_state_str(net_endpoint_state(base_underline)));
            return -1;
        }
    }
    else {
        net_endpoint_t base_endpoint = net_endpoint_from_data(underline->m_ws_endpoint);
        net_ws_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
        net_ws_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

        switch (net_endpoint_state(base_underline)) {
        case net_endpoint_state_resolving:
        case net_endpoint_state_connecting:
        case net_endpoint_state_deleting:
            assert(0);
            return 0;
        case net_endpoint_state_established:
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) {
                net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
                return 0;
            }
            return 0;
        case net_endpoint_state_error:
            net_endpoint_set_error(
                base_endpoint, net_endpoint_error_source(base_underline),
                net_endpoint_error_no(base_underline), net_endpoint_error_msg(base_underline));

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            }

            return 0;
        case net_endpoint_state_read_closed:
            return 0;
        case net_endpoint_state_write_closed:
            return 0;
        case net_endpoint_state_disable:
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
                return 0;
            }
            
            return 0;
        }
    }
}

int net_ws_svr_underline_write(
    net_endpoint_t base_underline, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf)
{
    net_ws_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ws_svr_driver_t driver = protocol->m_driver;
    net_ws_svr_underline_t underline = net_endpoint_protocol_data(base_underline);
    
    switch(net_endpoint_state(base_underline)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        if (net_endpoint_protocol_debug(base_underline)) {
            CPE_INFO(
                driver->m_em, "net: ws: %s: write: cache %d data in state %s!",
                net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline),
                net_endpoint_buf_size(from_ep, from_buf),
                net_endpoint_state_str(net_endpoint_state(base_underline)));
        }

        if (base_underline == from_ep) {
            return net_endpoint_buf_append_from_self(
                base_underline, net_ws_svr_underline_ep_write_cache, from_buf, 0);
        }
        else {
            return net_endpoint_buf_append_from_other(
                base_underline, net_ws_svr_underline_ep_write_cache, from_ep, from_buf, 0);
        }
    case net_endpoint_state_established:
        switch(underline->m_state) {
        case net_ws_svr_underline_ws_handshake:
            if (net_endpoint_protocol_debug(base_underline)) {
                CPE_INFO(
                    driver->m_em, "net: ws: %s: write: cache %d data in state %s.handshake!",
                    net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline),
                    net_endpoint_buf_size(from_ep, from_buf),
                    net_endpoint_state_str(net_endpoint_state(base_underline)));
            }

            if (base_underline == from_ep) {
                return net_endpoint_buf_append_from_self(
                    base_underline, net_ws_svr_underline_ep_write_cache, from_buf, 0);
            }
            else {
                return net_endpoint_buf_append_from_other(
                    base_underline, net_ws_svr_underline_ep_write_cache, from_ep, from_buf, 0);
            }
        case net_ws_svr_underline_ws_open:
            break;
        }
        break;
    case net_endpoint_state_error:
    case net_endpoint_state_deleting:
        CPE_ERROR(
            driver->m_em, "net: ws: %s: write: can`t write in state %s!",
            net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline),
            net_endpoint_state_str(net_endpoint_state(base_underline)));
        return -1;
    }

    uint32_t data_size = net_endpoint_buf_size(from_ep, from_buf);
    if (data_size == 0) return 0;

    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(from_ep, from_buf, data_size, &data) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ws: %s: write: peak data fail!",
            net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline));
        return -1;
    }

    /* int r = SSL_write(underline->m_ws, data, data_size); */
    /* if (r < 0) { */
    /*     int err = SSL_get_error(underline->m_ws, r); */
    /*     return net_ws_svr_underline_update_error(base_underline, err, r); */
    /* } */
    
    /* net_endpoint_buf_consume(from_ep, from_buf, r); */

    /* if (net_endpoint_protocol_debug(base_underline)) { */
    /*     CPE_INFO( */
    /*         driver->m_em, "net: ws: %s: ==> %d data!", */
    /*         net_endpoint_dump(net_ws_svr_driver_tmp_buffer(driver), base_underline), r); */
    /* } */
    
    return 0;
}

net_protocol_t
net_ws_svr_underline_protocol_create(net_schedule_t schedule, const char * name, net_ws_svr_driver_t driver) {
    net_protocol_t base_protocol =
        net_protocol_create(
            schedule,
            name,
            /*protocol*/
            sizeof(struct net_ws_svr_underline_protocol), NULL, NULL,
            /*endpoint*/
            sizeof(struct net_ws_svr_underline),
            net_ws_svr_underline_init,
            net_ws_svr_underline_fini,
            net_ws_svr_underline_input,
            net_ws_svr_underline_on_state_change,
            NULL);

    net_ws_svr_underline_protocol_t protocol = net_protocol_data(base_protocol);
    protocol->m_driver = driver;
    return base_protocol;
}

int net_ws_svr_underline_do_handshake(net_endpoint_t base_underline, net_ws_svr_underline_t underline) {
    return 0;
}
