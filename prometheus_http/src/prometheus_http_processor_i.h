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
    net_http_svr_protocol_t m_http_svr;
    net_http_svr_processor_t m_http_processor;
    struct mem_buffer m_collect_buffer;
};

#endif
