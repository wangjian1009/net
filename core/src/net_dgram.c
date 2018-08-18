#include "net_address.h"
#include "net_dgram_i.h"
#include "net_driver_i.h"

net_dgram_t net_dgram_create(
    net_driver_t driver,
    net_address_t address,
    net_dgram_process_fun_t process_fun, void * process_ctx)
{
    net_schedule_t schedule = driver->m_schedule;
    net_dgram_t dgram;

    dgram = TAILQ_FIRST(&driver->m_free_dgrams);
    if (dgram) {
        TAILQ_REMOVE(&driver->m_free_dgrams, dgram, m_next_for_driver);
    }
    else {
        dgram = mem_alloc(schedule->m_alloc, sizeof(struct net_dgram) + driver->m_dgram_capacity);
        if (dgram == NULL) {
            CPE_ERROR(schedule->m_em, "net_dgram_auto_create: alloc fail");
            return NULL;
        }
    }

    dgram->m_driver = driver;
    dgram->m_address = address;
    dgram->m_driver_debug = driver->m_debug;
    
    dgram->m_process_fun = process_fun;
    dgram->m_process_ctx = process_ctx;

    if (driver->m_dgram_init(dgram) != 0) {
        CPE_ERROR(schedule->m_em, "net_dgram_auto_create: init fail");
        TAILQ_INSERT_TAIL(&driver->m_free_dgrams, dgram, m_next_for_driver);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&driver->m_dgrams, dgram, m_next_for_driver);
    
    return dgram;
}

net_dgram_t net_dgram_auto_create(
    net_schedule_t schedule,
    net_address_t address,
    net_dgram_process_fun_t process_fun, void * process_ctx)
{
    net_driver_t driver;

    TAILQ_FOREACH(driver, &schedule->m_drivers, m_next_for_schedule) {
        if (driver->m_dgram_send) {
            return net_dgram_create(driver, address, process_fun, process_ctx);
        }
    }
    
    CPE_ERROR(schedule->m_em, "net_dgram_auto_create: no driver support dgram");
    return NULL;
}

void net_dgram_free(net_dgram_t dgram) {
    net_driver_t driver = dgram->m_driver;

    driver->m_dgram_fini(dgram);

    if (dgram->m_address) {
        net_address_free(dgram->m_address);
        dgram->m_address = NULL;
    }
    
    TAILQ_REMOVE(&driver->m_dgrams, dgram, m_next_for_driver);
    TAILQ_INSERT_TAIL(&driver->m_free_dgrams, dgram, m_next_for_driver);
}

void net_dgram_real_free(net_dgram_t dgram) {
    net_driver_t driver = dgram->m_driver;
    
    TAILQ_REMOVE(&driver->m_free_dgrams, dgram, m_next_for_driver);

    mem_free(driver->m_schedule->m_alloc, dgram);
}

net_schedule_t net_dgram_schedule(net_dgram_t dgram) {
    return dgram->m_driver->m_schedule;
}

net_driver_t net_dgram_driver(net_dgram_t dgram) {
    return dgram->m_driver;
}

net_address_t net_dgram_address(net_dgram_t dgram) {
    return dgram->m_address;
}

void net_dgram_set_address(net_dgram_t dgram, net_address_t address) {
    if (dgram->m_address) {
        net_address_free(dgram->m_address);
    }

    dgram->m_address = address;
}

uint8_t net_dgram_driver_debug(net_dgram_t dgram) {
    return dgram->m_driver_debug;
}

void net_dgram_set_driver_debug(net_dgram_t dgram, uint8_t debug) {
    dgram->m_driver_debug = debug;
}

void * net_dgram_data(net_dgram_t dgram) {
    return dgram + 1;
}

net_dgram_t net_dgram_from_data(void * data) {
    return ((net_dgram_t)data) - 1;
}

int net_dgram_send(net_dgram_t dgram, net_address_t target, void const * data, size_t data_size) {
    return dgram->m_driver->m_dgram_send(dgram, target, data, data_size);
}

void net_dgram_recv(net_dgram_t dgram, net_address_t from, void * data, size_t data_size) {
    if (dgram->m_process_fun) {
        dgram->m_process_fun(dgram, dgram->m_process_ctx, data, data_size, from);
    }
}

