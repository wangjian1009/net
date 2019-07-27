#ifndef NET_EBB_REQUEST_PARSER_H
#define NET_EBB_REQUEST_PARSER_H
#include "net_ebb_service_i.h"

typedef struct net_ebb_request_parser net_ebb_request_parser;

#define EBB_MAX_MULTIPART_BOUNDARY_LEN 20

struct net_ebb_request_parser {
    int cs; /* private */
    size_t chunk_size; /* private */
    unsigned eating : 1; /* private */
    net_ebb_request_t current_request; /* ro */
    const char* header_field_mark;
    const char* header_value_mark;
    const char* query_string_mark;
    const char* path_mark;
    const char* uri_mark;
    const char* fragment_mark;

    /* Public */
    net_ebb_request_t (*new_request)(void*);
    void* data;
};

void net_ebb_request_parser_init(net_ebb_request_parser* parser);
size_t net_ebb_request_parser_execute(net_ebb_request_parser* parser, const char* data, size_t len);
int net_ebb_request_parser_has_error(net_ebb_request_parser* parser);
int net_ebb_request_parser_is_finished(net_ebb_request_parser* parser);

#endif
