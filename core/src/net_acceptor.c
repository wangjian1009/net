#include "cpe/pal/pal_string.h"
#include "net_address.h"
#include "net_acceptor_i.h"
#include "net_protocol_i.h"

net_acceptor_t
net_acceptor_create(
    net_driver_t driver, net_protocol_t protocol,
    net_address_t address, uint8_t is_own, uint32_t accept_queue_size,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx)
{
    net_schedule_t schedule = driver->m_schedule;

    if (driver->m_acceptor_init == NULL) {
        CPE_ERROR(schedule->m_em, "core: acceptor: driver %s not support!", driver->m_name);
        return NULL;
    }
    
    net_acceptor_t acceptor = TAILQ_FIRST(&driver->m_free_acceptors);
    if (acceptor) {
        TAILQ_REMOVE(&driver->m_free_acceptors, acceptor, m_next_for_driver);
    }
    else {
        acceptor = mem_alloc(schedule->m_alloc, sizeof(struct net_acceptor) + driver->m_acceptor_capacity);
        if (acceptor == NULL) {
            CPE_ERROR(schedule->m_em, "core: acceptor: alloc fail!");
            return NULL;
        }
    }
    
    acceptor->m_driver = driver;
    acceptor->m_address = address;
    acceptor->m_protocol = protocol;
    acceptor->m_queue_size = accept_queue_size ? accept_queue_size : 512;
    acceptor->m_on_new_endpoint = on_new_endpoint;
    acceptor->m_on_new_endpoint_ctx = on_new_endpoint_ctx;

    if (is_own) {
        acceptor->m_address = address;
    }
    else {
        acceptor->m_address = net_address_copy(schedule, address);
        if (acceptor->m_address == NULL) {
            CPE_ERROR(schedule->m_em, "core: acceptor: dup address fail");
            TAILQ_INSERT_TAIL(&driver->m_free_acceptors, acceptor, m_next_for_driver);
            return NULL;
        }
    }
    
    if (driver->m_acceptor_init(acceptor) != 0) {
        CPE_ERROR(schedule->m_em, "core: acceptor: external init fail");
        if (!is_own) net_address_free(acceptor->m_address);
        TAILQ_INSERT_TAIL(&driver->m_free_acceptors, acceptor, m_next_for_driver);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&driver->m_acceptors, acceptor, m_next_for_driver);

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "core: acceptor: start at %s success. (protocol=%s, accept-queue-size=%d)",
            net_address_dump(net_schedule_tmp_buffer(schedule), acceptor->m_address),
            net_protocol_name(protocol),
            accept_queue_size);
    }
    
    return acceptor;
}

void net_acceptor_free(net_acceptor_t acceptor) {
    net_driver_t driver = acceptor->m_driver;
    
    driver->m_acceptor_fini(acceptor);
    
    if (acceptor->m_address) {
        net_address_free(acceptor->m_address);
        acceptor->m_address = NULL;
    }
    
    TAILQ_REMOVE(&driver->m_acceptors, acceptor, m_next_for_driver);

    TAILQ_INSERT_TAIL(&driver->m_free_acceptors, acceptor, m_next_for_driver);
}

void net_acceptor_real_free(net_acceptor_t acceptor) {
    net_driver_t driver = acceptor->m_driver;
    TAILQ_REMOVE(&driver->m_free_acceptors, acceptor, m_next_for_driver);
    mem_free(driver->m_schedule->m_alloc, acceptor);
}

net_schedule_t net_acceptor_schedule(net_acceptor_t acceptor) {
    return acceptor->m_driver->m_schedule;
}

net_driver_t net_acceptor_driver(net_acceptor_t acceptor) {
    return acceptor->m_driver;
}

net_protocol_t net_acceptor_protocol(net_acceptor_t acceptor) {
    return acceptor->m_protocol;
}

net_address_t net_acceptor_address(net_acceptor_t acceptor) {
    return acceptor->m_address;
}

uint32_t net_acceptor_queue_size(net_acceptor_t acceptor) {
    return acceptor->m_queue_size;
}

void * net_acceptor_data(net_acceptor_t acceptor) {
    return acceptor + 1;
}

net_acceptor_t net_acceptor_from_data(void * data) {
    return ((net_acceptor_t)data) - 1;
}

int net_acceptor_on_new_endpoint(net_acceptor_t acceptor, net_endpoint_t endpoint) {
    return acceptor->m_on_new_endpoint
        ? acceptor->m_on_new_endpoint(acceptor->m_on_new_endpoint_ctx, endpoint)
        : 0;
}
