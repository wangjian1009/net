#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_http2_endpoint.h"
#include "net_http2_req.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_stream_group_i.h"
#include "net_http2_stream_using_i.h"

int net_http2_stream_endpoint_select_control(net_endpoint_t base_endpoint);

int net_http2_stream_endpoint_init(net_endpoint_t base_endpoint) {
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_using = NULL;
    endpoint->m_req = NULL;
    return 0;
}

void net_http2_stream_endpoint_fini(net_endpoint_t base_endpoint) {
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_req) {
        net_http2_req_free(endpoint->m_req);
        endpoint->m_req = NULL;
    }

    if (endpoint->m_using) {
        net_http2_stream_using_free(endpoint->m_using);
        endpoint->m_using = NULL;
    }
}

net_http2_req_t
net_http2_stream_endpoint_req(net_http2_stream_endpoint_t endpoint) {
    return endpoint->m_req;
}

net_http2_endpoint_runing_mode_t
net_http2_stream_endpoint_runing_mode(net_http2_stream_endpoint_t endpoint) {
    return endpoint->m_using
        ? net_http2_endpoint_runing_mode(endpoint->m_using->m_http2_ep)
        : net_http2_endpoint_runing_mode_init;
}

int net_http2_stream_endpoint_connect(net_endpoint_t base_stream) {
    net_schedule_t schedule = net_endpoint_schedule(base_stream);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_stream));
    net_http2_stream_endpoint_t stream = net_endpoint_data(base_stream);

    if (stream->m_using == NULL) {
        if (net_http2_stream_endpoint_select_control(base_stream) != 0) return -1;
    }
    else {
        if (net_endpoint_remote_address(base_stream) == NULL) {
            net_endpoint_set_remote_address(
                base_stream,
                net_endpoint_remote_address(
                    net_http2_endpoint_base_endpoint(stream->m_using->m_http2_ep)));
        }
    }
    assert(stream->m_using);

    net_http2_endpoint_t http_ep = stream->m_using->m_http2_ep;
    net_endpoint_t base_http_ep = net_http2_endpoint_base_endpoint(http_ep);
    if (net_endpoint_driver_debug(base_stream) > net_endpoint_protocol_debug(base_http_ep)) {
        net_endpoint_set_protocol_debug(base_http_ep, net_endpoint_driver_debug(base_stream));
    }

    if (net_endpoint_state(base_http_ep) == net_endpoint_state_disable) {
        if (net_endpoint_connect(base_http_ep) != 0) {
            CPE_ERROR(
                driver->m_em, "http2: %s: %s: connect: start control connect fail!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_stream),
                net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(stream)));
            return -1;
        }
        assert(net_endpoint_state(base_http_ep) != net_endpoint_state_disable);
    }

    return net_endpoint_state(base_stream) == net_endpoint_state_disable
        ? net_http2_stream_endpoint_sync_state(stream)
        : 0;
}    

int net_http2_stream_endpoint_update(net_endpoint_t base_stream) {
    net_schedule_t schedule = net_endpoint_schedule(base_stream);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_stream));
    net_http2_stream_endpoint_t stream = net_endpoint_data(base_stream);

    switch(net_endpoint_state(base_stream)) {
    case net_endpoint_state_read_closed:
        return 0;
    case net_endpoint_state_write_closed:
        return 0;
    case net_endpoint_state_disable:
        return 0;
    case net_endpoint_state_established:
        return 0;
    case net_endpoint_state_error:
        return 0;
    default:
        return 0;
    }
}

int net_http2_stream_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    /* if (endpoint->m_control == NULL) { */
    /*     CPE_ERROR( */
    /*         driver->m_em, "http2: %s: %s: set no delay: no control!", */
    /*         net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint), */
    /*         net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint))); */
    /*     return -1; */
    /* } */

    return 0;
}

int net_http2_stream_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    /* if (endpoint->m_control == NULL) { */
    /*     CPE_ERROR( */
    /*         driver->m_em, "http2: %s: %s: get mss: no control", */
    /*         net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint), */
    /*         net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint))); */
    /*     return -1; */
    /* } */

    return 0;
}

net_http2_stream_endpoint_t
net_http2_stream_endpoint_create(net_http2_stream_driver_t driver, net_protocol_t protocol) {
    net_driver_t base_driver = net_driver_from_data(driver);
    
    net_endpoint_t base_endpoint = net_endpoint_create(base_driver, protocol, NULL);
    if (base_endpoint == NULL) return NULL;

    return net_endpoint_data(base_endpoint);
}

net_http2_stream_endpoint_t
net_http2_stream_endpoint_cast(net_endpoint_t base_endpoint) {
    net_driver_t driver = net_endpoint_driver(base_endpoint);
    return net_driver_endpoint_init_fun(driver) == net_http2_stream_endpoint_init
        ? net_endpoint_data(base_endpoint)
        : NULL;
}

net_endpoint_t
net_http2_stream_endpoint_base_endpoint(net_http2_stream_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

int net_http2_stream_endpoint_select_control(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    assert(endpoint->m_using == NULL);

    net_address_t target_addr = net_endpoint_remote_address(base_endpoint);
    if (target_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: select: target addr not set!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        return -1;
    }

    net_http2_stream_group_t remote = net_http2_stream_group_check_create(driver, target_addr);
    if (remote == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: select: check create remote faild!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        return -1;
    }

    net_endpoint_t base_http2_ep =
        net_endpoint_create(driver->m_control_driver, driver->m_control_protocol, NULL);
    if (base_http2_ep == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: select: create control ep fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        return -1;
    }

    if (net_endpoint_set_remote_address(base_http2_ep, target_addr) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: select: set control ep remote addr fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        net_endpoint_free(base_http2_ep);
        return -1;
    }

    net_http2_endpoint_t http2_ep = net_http2_endpoint_cast(base_http2_ep);
    if (net_http2_endpoint_set_runing_mode(http2_ep, net_http2_endpoint_runing_mode_cli) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: connect: set control runing mode cli failed!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        net_endpoint_free(base_http2_ep);
        return -1;
    }

    assert(endpoint->m_using == NULL);
    endpoint->m_using = net_http2_stream_using_create(&remote->m_usings, endpoint, http2_ep);
    if (endpoint->m_using == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: connect: create using failed!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        net_endpoint_free(base_http2_ep);
        return -1;
    }
    
    if (net_endpoint_driver_debug(base_endpoint) > net_endpoint_driver_debug(base_http2_ep)) {
        net_endpoint_set_driver_debug(base_http2_ep, net_endpoint_driver_debug(base_endpoint));
    }

    return 0;
}

int net_http2_stream_endpoint_sync_state(net_http2_stream_endpoint_t stream) {
    /* assert(stream->m_control); */
    /* net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint)); */
    /* net_endpoint_t control_base = stream->m_control->m_base_endpoint; */

    /* switch(net_endpoint_state(control_base)) { */
    /* case net_endpoint_state_resolving: */
    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_resolving) != 0) return -1; */
    /*     break; */
    /* case net_endpoint_state_connecting: */
    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_connecting) != 0) return -1; */
    /*     break; */
    /* case net_endpoint_state_established: */
    /*     if (net_http2_endpoint_runing_mode(stream->m_control) == net_http2_endpoint_runing_mode_cli) { */
    /*         if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_connecting) != 0) return -1; */

    /*         if (stream->m_control->m_state == net_http2_endpoint_state_streaming) { */
    /*             if (stream->m_control->m_runing_mode == net_http2_endpoint_runing_mode_cli) { */
    /*                 //if (net_http2_stream_endpoint_send_connect_request(stream) != 0) return -1; */
    /*             } */
    /*         } */
    /*     } */
    /*     else { */
    /*         assert(net_http2_endpoint_runing_mode(stream->m_control) == net_http2_endpoint_runing_mode_svr); */
    /*         if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_established) != 0) return -1; */
    /*     } */
    /*     break; */
    /* case net_endpoint_state_error: */
    /*     if (net_endpoint_error_source(stream->m_base_endpoint) == net_endpoint_error_source_none) { */
    /*         net_endpoint_set_error( */
    /*             stream->m_base_endpoint, */
    /*             net_endpoint_error_source(control_base), */
    /*             net_endpoint_error_no(control_base), */
    /*             net_endpoint_error_msg(control_base)); */
                
    /*     } */
    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_error) != 0) return -1; */
    /*     break; */
    /* case net_endpoint_state_read_closed: */
    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) return -1; */
    /*     break; */
    /* case net_endpoint_state_write_closed: */
    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) return -1; */
    /*     break; */
    /* case net_endpoint_state_disable: */
    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) return -1; */
    /*     break; */
    /* case net_endpoint_state_deleting: */
    /*     break; */
    /* } */

    return 0;
}
