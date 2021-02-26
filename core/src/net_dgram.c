#include "assert.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address.h"
#include "net_dgram_i.h"
#include "net_driver_i.h"
#include "net_debug_setup_i.h"

net_dgram_t net_dgram_create(
    net_driver_t driver,
    net_address_t address,
    net_dgram_process_fun_t process_fun, void * process_ctx)
{
    net_schedule_t schedule = driver->m_schedule;
    net_dgram_t dgram;

    assert(process_fun);

    dgram = TAILQ_FIRST(&driver->m_free_dgrams);
    if (dgram) {
        TAILQ_REMOVE(&driver->m_free_dgrams, dgram, m_next_for_driver);
    }
    else {
        dgram = mem_alloc(schedule->m_alloc, sizeof(struct net_dgram) + driver->m_dgram_capacity);
        if (dgram == NULL) {
            CPE_ERROR(schedule->m_em, "core: dgram: create: alloc fail");
            return NULL;
        }
    }

    dgram->m_id = ++schedule->m_dgram_max_id;
    dgram->m_driver = driver;
    dgram->m_address = NULL;
    dgram->m_driver_debug = driver->m_debug;
    dgram->m_is_processing = 0;
    dgram->m_is_free = 0;

    dgram->m_process_fun = process_fun;
    dgram->m_process_ctx = process_ctx;

    if (address) {
        dgram->m_address = net_address_copy(schedule, address);
        if (dgram->m_address == NULL) {
            CPE_ERROR(schedule->m_em, "core: dgram: create: dup address fail");
            return NULL;
        }
    }

    if (driver->m_dgram_init(dgram) != 0) {
        CPE_ERROR(schedule->m_em, "net_dgram_auto_create: init fail");
        if (dgram->m_address) net_address_free(dgram->m_address);
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

    if (dgram->m_is_processing) {
        dgram->m_is_free = 1;
        return;
    }
    
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

uint32_t net_dgram_id(net_dgram_t dgram) {
    return dgram->m_id;
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

int net_dgram_set_address(net_dgram_t dgram, net_address_t address) {
    if (dgram->m_address) {
        net_address_free(dgram->m_address);
        dgram->m_address = NULL;
    }

    if (address) {
        dgram->m_address = net_address_copy(dgram->m_driver->m_schedule, address);
        if (dgram->m_address == NULL) return -1;
    }

    return 0;
}

uint8_t net_dgram_driver_debug(net_dgram_t dgram) {
    return dgram->m_driver_debug;
}

void net_dgram_set_driver_debug(net_dgram_t dgram, uint8_t debug) {
    dgram->m_driver_debug = debug;
}

uint8_t net_dgram_protocol_debug(net_dgram_t dgram, net_address_t remote) {
    net_schedule_t schedule = dgram->m_driver->m_schedule;
    net_debug_setup_t debug_setup;
    uint8_t debug = 0;
    
    TAILQ_FOREACH(debug_setup, &schedule->m_debug_setups, m_next_for_schedule) {
        if (!net_debug_setup_check_dgram(debug_setup, dgram, remote)) continue;

        if (debug_setup->m_protocol_debug > debug) {
            debug = debug_setup->m_protocol_debug;
        }
    }

    return debug;
}

void * net_dgram_data(net_dgram_t dgram) {
    return dgram + 1;
}

net_dgram_t net_dgram_from_data(void * data) {
    return ((net_dgram_t)data) - 1;
}

int net_dgram_send(net_dgram_t dgram, net_address_t target, void const * data, size_t data_size) {
    uint8_t tag_local = 0;
    if (!dgram->m_is_processing) {
        tag_local = 0;
        dgram->m_is_processing = 1;
    }
    
    int rv = dgram->m_driver->m_dgram_send(dgram, target, data, data_size);

    if (tag_local) {
        dgram->m_is_processing = 0;
        if (dgram->m_is_free) {
            net_dgram_free(dgram);
        }
    }

    return rv;
}

void net_dgram_recv(net_dgram_t dgram, net_address_t from, void * data, size_t data_size) {
    uint8_t tag_local = 0;
    if (!dgram->m_is_processing) {
        tag_local = 0;
        dgram->m_is_processing = 1;
    }
    
    if (dgram->m_process_fun) {
        dgram->m_process_fun(dgram, dgram->m_process_ctx, data, data_size, from);
    }

    if (tag_local) {
        dgram->m_is_processing = 0;
        if (dgram->m_is_free) {
            net_dgram_free(dgram);
        }
    }
}

void net_dgram_print(write_stream_t ws, net_dgram_t dgram) {
    stream_printf(ws, "[%d: %s", dgram->m_id, dgram->m_driver->m_name);
    if (dgram->m_address) {
        stream_printf(ws, "@");
        net_address_print(ws, dgram->m_address);
    }
    stream_printf(ws, "]");
}

const char * net_dgram_dump(mem_buffer_t buff, net_dgram_t dgram) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buff);

    mem_buffer_clear_data(buff);
    net_dgram_print((write_stream_t)&stream, dgram);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buff, 0);
}

