#ifndef PROMETHEUS_MANAGER_I_H_INCLEDED
#define PROMETHEUS_MANAGER_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_ebb_system.h"
#include "prometheus_manager.h"

struct prometheus_manager {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_driver_t m_driver;
};
    
#endif
