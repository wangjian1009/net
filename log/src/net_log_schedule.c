#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_log_schedule_i.h"

net_log_schedule_t net_log_schedule_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t net_schedule, net_driver_t net_driver)
{
    net_log_schedule_t schedule = mem_alloc(alloc, sizeof(struct net_log_schedule));
    if (schedule == NULL) {
        CPE_ERROR(em, "log: schedule alloc fail");
        return NULL;
    }
    
    bzero(schedule, sizeof(*schedule));

    schedule->m_alloc = alloc;
    schedule->m_em = em;
    schedule->m_debug = 0;
    schedule->m_net_schedule = net_schedule;
    schedule->m_net_driver = net_driver;

    return schedule;
}

void net_log_schedule_free(net_log_schedule_t schedule) {
    mem_free(schedule->m_alloc, schedule);
}

uint8_t net_log_schedule_debug(net_log_schedule_t schedule) {
    return schedule->m_debug;
}

void net_log_schedule_set_debug(net_log_schedule_t schedule, uint8_t debug) {
    schedule->m_debug = debug;
}

mem_buffer_t net_log_schedule_tmp_buffer(net_log_schedule_t schedule) {
    return net_schedule_tmp_buffer(schedule->m_net_schedule);
}
