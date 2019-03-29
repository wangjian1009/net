#ifndef NET_LOG_UTIL_I_H_INCLEDED
#define NET_LOG_UTIL_I_H_INCLEDED
#include "net_log_schedule_i.h"

void md5_to_string(const char * buffer, int bufLen, char * md5);
int signature_to_base64(const char * sig, int sigLen, const char * key, int keyLen, char * base64);

#endif
