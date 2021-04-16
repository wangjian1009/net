#include "net_driver_i.h"
#include "net_progress_i.h"

net_progress_t
net_progress_auto_create(net_schedule_t schedule, const char * cmd, const char * mode) {
    net_driver_t driver;

    TAILQ_FOREACH(driver, &schedule->m_drivers, m_next_for_schedule) {
        if (driver->m_progress_init) {
            return net_progress_create(driver, cmd, mode);
        }
    }
    
    return NULL;
}

net_progress_t
net_progress_create(net_driver_t driver, const char * cmd, const char * mode) {
    net_schedule_t schedule = driver->m_schedule;

    net_progress_t progress = mem_alloc(schedule->m_alloc, sizeof(struct net_progress) + driver->m_progress_capacity);
    if (progress == NULL) {
        CPE_ERROR(schedule->m_em, "net: progress: create: alloc fail!");
        return NULL;
    }

    progress->m_driver = driver;
    if (driver->m_progress_init(progress, cmd, mode) != 0) {
        CPE_ERROR(schedule->m_em, "net: progress: create: start cmd fail, mode=%s, cmd=%s!", mode, cmd);
        mem_free(schedule->m_alloc, progress);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&driver->m_progresses, progress, m_next_for_driver);
    return progress;
}

void net_progress_free(net_progress_t progress) {
    net_driver_t driver = progress->m_driver;
    net_schedule_t schedule = driver->m_schedule;

    driver->m_progress_fini(progress);

    TAILQ_REMOVE(&driver->m_progresses, progress, m_next_for_driver);

    mem_free(schedule->m_alloc, progress);
}

net_driver_t net_progress_driver(net_progress_t progress) {
    return progress->m_driver;
}

void * net_progress_data(net_progress_t progress) {
    return progress + 1;
}

net_progress_t net_progress_from_data(void * data) {
    return ((net_progress_t)data) - 1;
}

