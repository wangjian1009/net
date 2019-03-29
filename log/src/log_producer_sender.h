//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_SENDER_H
#define LOG_C_SDK_LOG_PRODUCER_SENDER_H


#include "log_define.h"
#include "log_producer_config.h"
#include "log_builder.h"
#include "net_log_request.h"

LOG_CPP_START

#define LOG_PRODUCER_SEND_MAGIC_NUM 0x1B35487A

typedef int32_t log_producer_send_result;

extern void * log_producer_send_fun(void * send_param);

extern log_producer_result log_producer_send_data(net_log_request_param_t send_param);

extern log_producer_send_result AosStatusToResult(post_log_result * result);

LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_SENDER_H
