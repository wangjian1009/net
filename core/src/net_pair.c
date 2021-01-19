#include <assert.h>
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_timer.h"
#include "net_pair_i.h"

void net_pair_endpoint_delay_process(net_timer_t timer, void * ctx);

net_driver_t net_schedule_pair_driver(net_schedule_t schedule) {
    return schedule->m_pair_driver;
}

int net_pair_endpoint_make(net_schedule_t schedule, net_protocol_t p0, net_protocol_t p1, net_endpoint_t ep[2]) {
    net_endpoint_t endpoint_a = net_endpoint_create(schedule->m_pair_driver, p0, NULL);
    if (endpoint_a == NULL) {
        CPE_ERROR(
            schedule->m_em, "core: pair: make: create endpoint a failed, protocol=%s",
            net_protocol_name(p0));
        return -1;
    }
        
    net_endpoint_t endpoint_z = net_endpoint_create(schedule->m_pair_driver, p1, NULL);
    if (endpoint_z == NULL) {
        CPE_ERROR(
            schedule->m_em, "core: pair: make: create endpoint z failed, protocol=%s",
            net_protocol_name(p1));
        net_endpoint_free(endpoint_a);
        return -1;
    }

    if (net_pair_endpoint_link(endpoint_a, endpoint_z) != 0) {
        net_endpoint_free(endpoint_a);
        net_endpoint_free(endpoint_z);
        return -1;
    }

    ep[0] = endpoint_a;
    ep[1] = endpoint_z;

    return 0;
}

net_endpoint_t net_pair_endpoint_other(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);

    if (net_endpoint_driver(base_endpoint) != schedule->m_pair_driver) {
        CPE_ERROR(
            schedule->m_em, "core: pair: other: driver %s is not pair driver, no other",
            net_endpoint_driver_name(base_endpoint));
        return NULL;
    }

    net_pair_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    return endpoint->m_other ? net_endpoint_from_data(endpoint->m_other) : NULL;
}

int net_pair_endpoint_link(net_endpoint_t base_a, net_endpoint_t base_z) {
    net_schedule_t schedule = net_endpoint_schedule(base_a);

    /*检查a*/
    if (net_endpoint_driver(base_a) != schedule->m_pair_driver) {
        CPE_ERROR(
            schedule->m_em, "core: pair: link: %s: %s is not pair driver, can`t link",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_a),
            net_endpoint_driver_name(base_a));
        return -1;
    }

    if (net_endpoint_state(base_a) != net_endpoint_state_disable) {
        CPE_ERROR(
            schedule->m_em, "core: pair: link: %s: state is %s, can`t link",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_a),
            net_endpoint_state_str(net_endpoint_state(base_a)));
        return -1;
    }

    net_pair_endpoint_t a = net_endpoint_data(base_a);
    if (a->m_other != NULL) {
        CPE_ERROR(
            schedule->m_em, "core: pair: link: %s: already have linked ep, can`t link",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_a));
        return -1;
    }
    
    /*检查z*/
    if (net_endpoint_driver(base_z) != schedule->m_pair_driver) {
        CPE_ERROR(
            schedule->m_em, "core: pair: link: %s: %s is not pair driver, can`t link",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_z),
            net_endpoint_driver_name(base_z));
        return -1;
    }

    if (net_endpoint_state(base_z) != net_endpoint_state_disable) {
        CPE_ERROR(
            schedule->m_em, "core: pair: link: %s: state is %s, can`t link",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_z),
            net_endpoint_state_str(net_endpoint_state(base_z)));
        return -1;
    }
    
    net_pair_endpoint_t z = net_endpoint_data(base_z);
    if (z->m_other != NULL) {
        CPE_ERROR(
            schedule->m_em, "core: pair: link: %s: already have linked ep, can`t link",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_z));
        return -1;
    }

    assert(a->m_other == NULL);
    a->m_other = z;

    assert(z->m_other == NULL);
    z->m_other = a;

    if (net_endpoint_set_state(base_a, net_endpoint_state_established) != 0) {
        a->m_other = NULL;
        z->m_other = NULL;
        return -1;
    }

    if (net_endpoint_set_state(base_z, net_endpoint_state_established) != 0) {
        a->m_other = NULL;
        z->m_other = NULL;
        return -1;
    }
    
    return 0;
}

int net_pair_endpoint_init(net_endpoint_t base_endpoint) {
    net_pair_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_other = NULL;
    endpoint->m_delay_processor = NULL;
    endpoint->m_is_writing = 0;
    return 0;
}

void net_pair_endpoint_fini(net_endpoint_t base_endpoint) {
    net_pair_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_delay_processor) {
        net_timer_free(endpoint->m_delay_processor);
    }
    
    if (endpoint->m_other) {
        net_pair_endpoint_t other = endpoint->m_other;
        net_endpoint_t base_other = net_endpoint_from_data(other);
        
        assert(other->m_other == endpoint);
        
        other->m_other = NULL;
        endpoint->m_other = NULL;
        
        if (net_endpoint_set_state(base_other, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(base_other, net_endpoint_state_deleting);
        }
    }
    assert(endpoint->m_other == NULL);
}

int net_pair_endpoint_update(net_endpoint_t base_endpoint) {
    net_pair_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);

    if (endpoint->m_is_writing) return 0;

    while(!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) {
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return -1;

        if (endpoint->m_other == NULL) {
            CPE_ERROR(
                schedule->m_em, "core: pair: %s: other end disconnected",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
            net_endpoint_set_error(
                base_endpoint,
                net_endpoint_error_source_network, net_endpoint_network_errno_logic,
                "pair other disconnected");
            return net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error);
        }

        if (endpoint->m_other->m_is_writing) {
            if (endpoint->m_delay_processor == NULL) {
                endpoint->m_delay_processor =
                    net_timer_auto_create(schedule, net_pair_endpoint_delay_process, base_endpoint);
                if (endpoint->m_delay_processor == NULL) {
                    CPE_ERROR(
                        schedule->m_em, "core: pair: %s: create delay processor failed",
                        net_endpoint_dump(net_schedule_tmp_buffer(schedule), base_endpoint));
                    return -1;
                }
            }
            net_timer_active(endpoint->m_delay_processor, 0);
            return 0;
        }

        net_endpoint_t base_other = net_endpoint_from_data(endpoint->m_other);
        endpoint->m_is_writing = 1;
        int rv = net_endpoint_buf_append_from_other(base_other, net_ep_buf_read, base_endpoint, net_ep_buf_write, 0);
        endpoint->m_is_writing = 0;
        if (rv != 0) return rv;
    }
    
    return 0;
}

void net_pair_endpoint_close(net_endpoint_t base_endpoint) {
    net_pair_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_other) {
        net_pair_endpoint_t other = endpoint->m_other;
        net_endpoint_t base_other = net_endpoint_from_data(other);
        
        assert(other->m_other == endpoint);
        
        other->m_other = NULL;
        endpoint->m_other = NULL;
        
        if (net_endpoint_set_state(base_other, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(base_other, net_endpoint_state_deleting);
        }
    }
    assert(endpoint->m_other == NULL);
}

int net_pair_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    return 0;
}

int net_pair_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    *mss = 0;
    return 0;
}

void net_pair_endpoint_delay_process(net_timer_t timer, void * ctx) {
    net_endpoint_t base_endpoint = ctx;

    if (net_pair_endpoint_update(base_endpoint) != 0) {
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) {
            net_endpoint_set_error(
                base_endpoint,
                net_endpoint_error_source_network, net_endpoint_network_errno_logic,
                "unknown error in delay process");
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                net_endpoint_free(base_endpoint);
            }
        }
    }
}

net_driver_t net_pair_driver_create(net_schedule_t schedule) {
    return net_driver_create(
        schedule,
        "pair",
        /*driver*/
        0, NULL, NULL, NULL,
        /*timer*/
        0, NULL, NULL, NULL, NULL, NULL,
        /*acceptor*/
        0, NULL, NULL,
        /*endpoint*/
        sizeof(struct net_pair_endpoint),
        net_pair_endpoint_init,
        net_pair_endpoint_fini,
        NULL,
        net_pair_endpoint_close,
        net_pair_endpoint_update,
        net_pair_endpoint_set_no_delay,
        net_pair_endpoint_get_mss,
        /*dgram*/
        0, NULL, NULL, NULL,
        /*watcher*/
        0, NULL, NULL, NULL);
}
