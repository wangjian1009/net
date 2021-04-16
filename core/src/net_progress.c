#include "net_progress_i.h"

net_progress_t net_progress_auto_create(net_schedule_t schedule, const char * cmd, const char * mode) {
    net_driver_t driver;

    TAILQ_FOREACH(driver, &schedule->m_drivers, m_next_for_schedule) {
        if (driver->m_progress) {
            return net_progress_create(driver, process_fun, process_ctx);
        }
    }
    
    return NULL;
}

net_progress_t net_progress_create(net_driver_t driver, const char * cmd, const char * mode) {
}

void net_progress_free(net_progress_t progress);

net_driver_t net_progress_driver(net_progress_t progress) {
}

void * net_progress_data(net_progress_t progress) {
    return progress + 1;
}

net_progress_t net_progress_from_data(void * data) {
    return ((net_progress_t)data) - 1;
}

