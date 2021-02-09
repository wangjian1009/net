#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_stream_remote_i.h"
#include "net_http2_endpoint_i.h"

int net_http2_stream_endpoint_select_control(net_endpoint_t base_endpoint);
void net_http2_stream_endpoint_update_readable(net_endpoint_t base_endpoint);

int net_http2_stream_endpoint_init(net_endpoint_t base_endpoint) {
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_control = NULL;
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
            driver->m_em, "http2: stream: %s: connect: set control runing mode cli failed!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }
    
    net_endpoint_t base_control = endpoint->m_control->m_base_endpoint;
    if (net_endpoint_driver_debug(base_endpoint) > net_endpoint_protocol_debug(base_control)) {
        net_endpoint_set_protocol_debug(base_control, net_endpoint_driver_debug(base_endpoint));
    }

CHECK_STATE_AGAIN:
    switch(net_endpoint_state(base_control)) {
    case net_endpoint_state_disable:
        if (net_endpoint_connect(base_control) != 0) {
            CPE_ERROR(
                driver->m_em, "http2: stream: %s: connect: start control connect fail!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
            return -1;
        }
        assert(net_endpoint_state(base_control) != net_endpoint_state_disable);
        goto CHECK_STATE_AGAIN;
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
    case net_endpoint_state_established:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) {
            CPE_ERROR(
                driver->m_em, "http2: stream: %s: connect: set state to %s failed!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
                net_endpoint_state_str(net_endpoint_state_connecting));
            return -1;
        }
        break;
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
    case net_endpoint_state_error:
    case net_endpoint_state_deleting:
        CPE_ERROR(
            driver->m_em, "http2: stream: %s: connect: can`t reuse control in state %s!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint),
            net_endpoint_state_str(net_endpoint_state(base_control)));
        return -1;
    }

    return 0;
}    

int net_http2_stream_endpoint_update(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_endpoint_t base_control = endpoint->m_control ? endpoint->m_control->m_base_endpoint : NULL;
    
    if (base_control) {
        net_http2_stream_endpoint_update_readable(base_endpoint);
    }

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_read_closed:
        if (base_control && net_endpoint_is_active(base_control)) {
            if (net_endpoint_set_state(base_control, net_endpoint_state_read_closed) != 0) {
                net_endpoint_set_state(base_control, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_write_closed:
        if (base_control && net_endpoint_is_active(base_control)) {
            if (net_endpoint_set_state(base_control, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(base_control, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_disable:
        if (base_control && net_endpoint_is_active(base_control)) {
            if (net_endpoint_set_state(base_control, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(base_control, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_established:
        if (base_control == NULL) {
            CPE_ERROR(
                driver->m_em, "http2: stream: %s: set no delay: no control!",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
            return -1;
        }

        if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) { /*有数据等待写入 */
            uint32_t buf_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
            void *  buf = NULL;
            if (net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_write, buf_size, &buf) != 0) {
                CPE_ERROR(
                    driver->m_em, "http2: stream: %s: peak data to send fail!, size=%d!",
                    net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint), buf_size);
                return -1;
            }

            if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                char name_buf[128];
                cpe_str_dup(name_buf, sizeof(name_buf), net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint));
                CPE_INFO(
                    driver->m_em, "http2: stream: %s: ==> %d data\n%s",
                    name_buf, buf_size,
                    mem_buffer_dump_data(net_http2_stream_driver_tmp_buffer(driver), buf, buf_size, 0));
            }
            
            /* if (net_http2_endpoint_send_msg_bin(endpoint->m_control, buf, buf_size) != 0) { */
            /*     CPE_ERROR( */
            /*         driver->m_em, "http2: stream: %s: send bin message fail, size=%d!", */
            /*         net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint), buf_size); */
            /*     return -1; */
            /* } */

            if (net_endpoint_state(base_endpoint) == net_endpoint_state_deleting) return -1;

            net_endpoint_buf_consume(base_endpoint, net_ep_buf_write, buf_size);
            
            if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
        }

        assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);
        return 0;
    case net_endpoint_state_error:
        if (base_control && net_endpoint_is_active(base_control)) {
            if (net_endpoint_set_state(base_control, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_control, net_endpoint_state_deleting);
            }
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
            driver->m_em, "http2: stream: %s: set no delay: no control!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    return net_endpoint_set_no_delay(endpoint->m_control->m_base_endpoint, no_delay);
}

int net_http2_stream_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_control == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: stream: %s: get mss: no control",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint));
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

int net_http2_stream_endpoint_select_control(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    assert(endpoint->m_control == NULL);

    net_address_t target_addr = net_endpoint_remote_address(base_endpoint);
    if (target_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: stream: %s: select: target addr not set!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    net_http2_stream_remote_t remote = net_http2_stream_remote_check_create(driver, target_addr);
    if (remote == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: stream: %s: select: check create remote faild!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    net_endpoint_t base_control =
        net_endpoint_create(driver->m_control_driver, driver->m_control_protocol, NULL);
    if (base_control == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: stream: %s: select: create control ep fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
        return -1;
    }

    if (net_endpoint_set_remote_address(base_control, target_addr) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: stream: %s: select: set control ep remote addr fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
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

void net_http2_stream_endpoint_update_readable(net_endpoint_t base_endpoint) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_http2_stream_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_endpoint_t base_control = endpoint->m_control ? endpoint->m_control->m_base_endpoint : NULL;
    
    assert(base_control != NULL);

    if (net_endpoint_expect_read(base_endpoint)) {
        if (!net_endpoint_expect_read(base_control)) {
            if (net_endpoint_driver_debug(base_endpoint) >= 3) {
                CPE_INFO(
                    driver->m_em, "http2: stream: %s: read begin!",
                    net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint));
            }

            net_endpoint_set_expect_read(base_control, 1);
        }
    } else {
        if (net_endpoint_expect_read(base_control)) {
            if (net_endpoint_driver_debug(base_endpoint) >= 3) {
                CPE_INFO(
                    driver->m_em, "http2: stream: %s: read stop!",
                    net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), base_endpoint));
            }

            net_endpoint_set_expect_read(base_control, 0);
        }
    }
}

int net_http2_stream_endpoint_on_head(
    net_http2_stream_endpoint_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len)
{
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));

    if (cpe_str_cmp_part((const char *)name, name_len, ":status") == 0) {
        char value_buf[64];
        if (value_len + 1 > CPE_ARRAY_SIZE(value_buf)) {
            CPE_ERROR(
                driver->m_em, "http2: %s: on head: status overflow!",
                net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint));
            return -1;
        }

        const char * str_status = cpe_str_dup_len(value_buf, sizeof(value_buf), value, value_len);
        int status = atoi(str_status);
        if (status == 200) {
            return 0;
        }
        else {
            CPE_ERROR(
                driver->m_em, "http2: %s: on head: direct fail, status=%.*s!",
                net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
                (int)value_len, value);
            return -1;
        }
    }
    
    return 0;
}

int net_http2_stream_endpoint_on_tailer(
    net_http2_stream_endpoint_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len)
{
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));

    if (cpe_str_cmp_part((const char *)name, name_len, "reason") == 0) {
        //assert(stream->m_conn);

        /* if (sfox_client_conn_close_reason(stream->m_conn) == sfox_client_conn_close_reason_none) { */
        /*     char buf[128]; */
            
        /*     sfox_client_conn_set_close_reason( */
        /*         stream->m_conn, */
        /*         sfox_client_conn_close_reason_remote_close, */
        /*         cpe_str_dup_len(buf, sizeof(buf), value, value_len), NULL); */
        /* } */
    }

    return 0;
}

