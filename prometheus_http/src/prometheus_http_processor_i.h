#ifndef PROMETHEUS_HTTP_PROCESSOR_I_H_INCLEDED
#define PROMETHEUS_HTTP_PROCESSOR_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "prometheus_http_processor.h"

struct prometheus_http_processor {
    error_monitor_t m_em;
    mem_allocrator_t m_alloc;
    prometheus_manager_t m_manager;
};

#endif
