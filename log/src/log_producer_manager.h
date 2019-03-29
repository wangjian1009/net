#ifndef NET_LOG_MANAGER_I_H_INCLEDED
#define NET_LOG_MANAGER_I_H_INCLEDED
#include "net_log_category_i.h"
#include "log_define.h"
#include "log_producer_config.h"
#include "log_producer_sender.h"
#include "log_builder.h"
#include "log_queue.h"

typedef struct _log_producer_manager
{
    net_log_category_t m_category;
    volatile uint32_t networkRecover;
    volatile uint32_t totalBufferSize;
    log_group_builder * builder;
    int32_t firstLogTime;
    char * source;
    char * pack_prefix;
    volatile uint32_t pack_index;
} log_producer_manager;

extern log_producer_manager * create_log_producer_manager(net_log_category_t category);
extern void destroy_log_producer_manager(log_producer_manager * manager);

extern log_producer_result log_producer_manager_add_log(log_producer_manager * producer_manager, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens);

#endif //LOG_C_SDK_LOG_PRODUCER_MANAGER_H
