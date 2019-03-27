#ifndef NET_LOG_CATEGORY_I_H_INCLEDED
#define NET_LOG_CATEGORY_I_H_INCLEDED
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_category {
    net_log_schedule_t m_schedule;
    uint8_t m_id;
    log_producer_t m_log_producer;
};

NET_END_DECL

#endif

