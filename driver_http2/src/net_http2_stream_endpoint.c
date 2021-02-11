#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_stream_group_i.h"
#include "net_http2_endpoint_i.h"

int net_http2_stream_endpoint_select_control(net_endpoint_t base_endpoint);

int net_http2_stream_endpoint_init(net_endpoint_t base_endpoint) {
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_control = NULL;
    endpoint->m_state = net_http2_stream_endpoint_state_init;
    endpoint->m_send_scheduled = 0;
    endpoint->m_send_processing = 0;
    endpoint->m_stream_id = -1;
    return 0;
}

void net_http2_stream_endpoint_fini(net_endpoint_t base_endpoint) {
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_control) {
        net_http2_stream_endpoint_set_control(endpoint, NULL);
    }

    assert(endpoint->m_stream_id == -1);
}

net_http2_stream_endpoint_state_t
net_http2_stream_endpoint_state(net_http2_stream_endpoint_t endpoint) {
    return endpoint->m_state;
}

net_http2_endpoint_runing_mode_t
net_http2_stream_endpoint_runing_mode(net_http2_stream_endpoint_t endpoint) {
    return endpoint->m_control
        ? net_http2_endpoint_runing_mode(endpoint->m_control)
        : net_http2_endpoint_runing_mode_init;
}

int net_http2_stream_endpoint_connect(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_control == NULL) {
        if (net_http2_stream_endpoint_select_control(base_endpoint) != 0) return -1;
    }
    else {
        if (net_endpoint_remote_address(base_endpoint) == NULL) {
            net_endpoint_set_remote_address(
                base_endpoint,
                net_endpoint_remote_address(endpoint->m_control->m_base_endpoint));
        }
    }

    if (net_http2_endpoint_set_runing_mode(endpoint->m_control, net_http2_endpoint_runing_mode_cli) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: connect: set control runing mode cli failed!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        return -1;
    }
    
    net_endpoint_t base_control = endpoint->m_control->m_base_endpoint;
    if (net_endpoint_driver_debug(base_endpoint) > net_endpoint_protocol_debug(base_control)) {
        net_endpoint_set_protocol_debug(base_control, net_endpoint_driver_debug(base_endpoint));
    }

    if (net_endpoint_state(base_control) == net_endpoint_state_disable) {
        if (net_endpoint_connect(base_control) != 0) {
            CPE_ERROR(
                driver->m_em, "http2: %s: %s: connect: start control connect fail!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
                net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
            return -1;
        }
        assert(net_endpoint_state(base_control) != net_endpoint_state_disable);
    }

    return net_endpoint_state(base_endpoint) == net_endpoint_state_disable
        ? net_http2_stream_endpoint_sync_state(endpoint)
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
        if (stream->m_stream_id != -1) {
            
        }
        return 0;
    default:
        return 0;
    }
}

int net_http2_stream_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_control == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: set no delay: no control!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        return -1;
    }

    return net_endpoint_set_no_delay(endpoint->m_control->m_base_endpoint, no_delay);
}

int net_http2_stream_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_control == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: get mss: no control",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        return -1;
    }

    return net_endpoint_get_mss(endpoint->m_control->m_base_endpoint, mss);
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

net_http2_endpoint_t
net_http2_stream_endpoint_control(net_http2_stream_endpoint_t endpoint) {
    return endpoint->m_control;
}

net_endpoint_t
net_http2_stream_endpoint_base_endpoint(net_http2_stream_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

int32_t net_http2_stream_endpoint_stream_id(net_http2_stream_endpoint_t endpoint) {
    return endpoint->m_stream_id;
}

net_http2_stream_endpoint_t
net_http2_stream_endpoint_find_by_stream_id(net_http2_endpoint_t endpoint, int32_t stream_id) {
    net_http2_stream_endpoint_t stream;

    TAILQ_FOREACH(stream, &endpoint->m_streams, m_next_for_control) {
        if (stream->m_stream_id == stream_id) return stream;
    }

    return NULL;
}

void net_http2_stream_endpoint_set_control(net_http2_stream_endpoint_t endpoint, net_http2_endpoint_t control) {
    if (endpoint->m_control == control) return;

    if (endpoint->m_control) {
        if (endpoint->m_stream_id) {
            if (net_endpoint_state(endpoint->m_control->m_base_endpoint) == net_endpoint_state_established) {
            }
            endpoint->m_stream_id = -1;
        }
        
        TAILQ_REMOVE(&control->m_streams, endpoint, m_next_for_control);
    }

    endpoint->m_control = control;

    if (endpoint->m_control) {
        TAILQ_INSERT_TAIL(&control->m_streams, endpoint, m_next_for_control);
    }
}

int net_http2_stream_endpoint_set_state(net_http2_stream_endpoint_t stream, net_http2_stream_endpoint_state_t state) {
    if (stream->m_state == state) return 0;

    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));

    if (net_endpoint_driver_debug(stream->m_base_endpoint)) {
        CPE_INFO(
            driver->m_em, "http2: %s: stream-state: %s ==> %s",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_stream_endpoint_state_str(stream->m_state),
            net_http2_stream_endpoint_state_str(state));
    }

    stream->m_state = state;

    switch(stream->m_state) {
    case net_http2_stream_endpoint_state_init:
        break;
    case net_http2_stream_endpoint_state_connecting:
        if (stream->m_control->m_runing_mode == net_http2_endpoint_runing_mode_cli) {
            if (net_http2_stream_endpoint_send_connect_request(stream) != 0) return -1;
        }
        break;
    case net_http2_stream_endpoint_state_established:
        break;
    }
    
    return 0;
}

int net_http2_stream_endpoint_select_control(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    assert(endpoint->m_control == NULL);

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

    net_endpoint_t base_control =
        net_endpoint_create(driver->m_control_driver, driver->m_control_protocol, NULL);
    if (base_control == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: select: create control ep fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        return -1;
    }

    if (net_endpoint_set_remote_address(base_control, target_addr) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: select: set control ep remote addr fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(endpoint)));
        net_endpoint_free(base_endpoint);
        return -1;
    }
    
    net_http2_endpoint_t control = net_http2_endpoint_cast(base_control);
    net_http2_stream_endpoint_set_control(endpoint, control);

    if (net_endpoint_driver_debug(base_endpoint) > net_endpoint_driver_debug(base_control)) {
        net_endpoint_set_driver_debug(base_control, net_endpoint_driver_debug(base_endpoint));
    }

    return 0;
}

const char * net_http2_stream_endpoint_state_str(net_http2_stream_endpoint_state_t state) {
    switch(state) {
    case net_http2_stream_endpoint_state_init:
        return "init";
    case net_http2_stream_endpoint_state_connecting:
        return "connecting";
    case net_http2_stream_endpoint_state_established:
        return "established";
    }
}
