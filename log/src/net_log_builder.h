#ifndef NET_LOG_BUILDER_H_INCLEDED
#define NET_LOG_BUILDER_H_INCLEDED
#include "net_log_schedule_i.h"

typedef struct _log_tag {
    char * buffer;
    char * now_buffer;
    uint32_t max_buffer_len;
    uint32_t now_buffer_len;
} log_tag;

typedef struct _log_group {
    char * source;
    char * topic;
    log_tag tags;
    log_tag logs;
    uint32_t n_logs;
    char * log_now_buffer;
}log_group;

struct net_log_builder {
    net_log_category_t m_category;
    log_group* grp;
    size_t loggroup_size;
    uint32_t builder_time;
};

typedef struct _log_buffer {
    char * buffer;
    uint32_t n_buffer;
} log_buf;

extern log_buf serialize_to_proto_buf_with_malloc(net_log_builder_t bder);
extern net_log_lz4_buf_t serialize_to_proto_buf_with_malloc_lz4(net_log_builder_t bder);
extern net_log_lz4_buf_t serialize_to_proto_buf_with_malloc_no_lz4(net_log_builder_t bder);

extern void lz4_log_buf_free(net_log_lz4_buf_t pBuf);
extern net_log_lz4_buf_t lz4_log_buf_create(net_log_schedule_t schedule, void const * data, uint32_t length, uint32_t raw_length);

extern net_log_builder_t log_group_create(net_log_category_t category);
extern void net_log_group_destroy(net_log_builder_t bder);
extern void add_source(net_log_builder_t bder,const char* src,size_t len);
extern void add_topic(net_log_builder_t bder,const char* tpc,size_t len);
extern void add_tag(net_log_builder_t bder,const char* k,size_t k_len,const char* v,size_t v_len);
extern void add_pack_id(net_log_builder_t bder, const char* pack, size_t pack_len, size_t packNum);
extern void fix_log_group_time(char * pb_buffer, size_t len, uint32_t new_time);

extern void add_log_begin(net_log_builder_t bder);
extern void add_log_time(net_log_builder_t bder, uint32_t logTime);
extern void add_log_key_value(net_log_builder_t bder, char * key, size_t key_len, char * value, size_t value_len);
extern void add_log_end(net_log_builder_t bder);

#endif
