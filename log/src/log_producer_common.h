#ifndef LOG_C_SDK_LOG_PRODUCER_COMMON_H_H
#define LOG_C_SDK_LOG_PRODUCER_COMMON_H_H

#include <stdint.h>
#include <stddef.h>


/**
 * log producer result for all operation
 */
typedef int log_producer_result;

/**
 * callback function for producer client
 * @param config_name this is logstore name
 * @param result send result
 * @param log_bytes log group packaged bytes
 * @param compressed_bytes lz4 compressed bytes
 * @param req_id request id. Must check NULL when use it
 * @param error_message if send result is not ok, error message is set. Must check NULL when use it
 * @param raw_buffer lz4 buffer
 * @note you can only read raw_buffer, but can't modify or free it
 */
extern log_producer_result LOG_PRODUCER_OK;
extern log_producer_result LOG_PRODUCER_INVALID;
extern log_producer_result LOG_PRODUCER_WRITE_ERROR;
extern log_producer_result LOG_PRODUCER_DROP_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_NETWORK_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_QUOTA_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_UNAUTHORIZED;
extern log_producer_result LOG_PRODUCER_SEND_SERVER_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_DISCARD_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_TIME_ERROR;
// this error code is passed when producer is being destroyed, you should save this buffer to local
extern log_producer_result LOG_PRODUCER_SEND_EXIT_BUFFERED;

#endif //LOG_C_SDK_LOG_PRODUCER_COMMON_H_H
