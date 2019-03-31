#ifndef NET_LOG_BUILDER_H_INCLEDED
#define NET_LOG_BUILDER_H_INCLEDED
#include "net_log_schedule_i.h"

typedef struct _log_tag {
    char * buffer;
    char * now_buffer;
    uint32_t max_buffer_len;
    uint32_t now_buffer_len;
} log_tag;

typedef struct _log_group{
    char * source;
    char * topic;
    log_tag tags;
    log_tag logs;
    size_t n_logs;
#ifdef LOG_KEY_VALUE_FLAG
    char * log_now_buffer;
#endif
}log_group;

struct net_log_lz4_buf {
    size_t length;
    size_t raw_length;
    unsigned char data[0];
};

struct net_log_group_builder {
    net_log_category_t m_category;
    log_group* grp;
    size_t loggroup_size;
    uint32_t builder_time;
};

typedef struct _log_buffer {
    char * buffer;
    uint32_t n_buffer;
} log_buf;

extern log_buf serialize_to_proto_buf_with_malloc(net_log_group_builder_t bder);
extern net_log_lz4_buf_t serialize_to_proto_buf_with_malloc_lz4(net_log_group_builder_t bder);
extern net_log_lz4_buf_t serialize_to_proto_buf_with_malloc_no_lz4(net_log_group_builder_t bder);
extern void free_lz4_log_buf(net_log_lz4_buf_t pBuf);
extern net_log_group_builder_t log_group_create(net_log_category_t category);
extern void net_log_group_destroy(net_log_group_builder_t bder);
extern void add_log_full(net_log_group_builder_t bder, uint32_t logTime, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens);
extern void add_source(net_log_group_builder_t bder,const char* src,size_t len);
extern void add_topic(net_log_group_builder_t bder,const char* tpc,size_t len);
extern void add_tag(net_log_group_builder_t bder,const char* k,size_t k_len,const char* v,size_t v_len);
extern void add_pack_id(net_log_group_builder_t bder, const char* pack, size_t pack_len, size_t packNum);
extern void fix_log_group_time(char * pb_buffer, size_t len, uint32_t new_time);

// if you want to use this functions, add `-DADD_LOG_KEY_VALUE_FUN=ON` for cmake. eg `cmake . -DADD_LOG_KEY_VALUE_FUN=ON`
#ifdef LOG_KEY_VALUE_FLAG
/**
 * call it when you want to add a new log
 * @note sequence must be : add_log_begin -> add_log_time/add_log_key_value..... -> add_log_end
 * @param bder
 */
extern void add_log_begin(net_log_group_builder_t bder);

/**
 * set log's time, must been called only once in one log
 * @param bder
 * @param logTime
 */
extern void add_log_time(net_log_group_builder_t bder, uint32_t logTime);

/**
 * add key&value pair to log tail
 * @param bder
 * @param key
 * @param key_len
 * @param value
 * @param value_len
 */
extern void add_log_key_value(net_log_group_builder_t bder, char * key, size_t key_len, char * value, size_t value_len);

/**
 * add log end, call it when you add time and key&value done
 * @param bder
 */
extern void add_log_end(net_log_group_builder_t bder);

#endif

#endif
