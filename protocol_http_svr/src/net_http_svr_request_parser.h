#ifndef NET_HTTP_SVR_REQUEST_PARSER_H
#define NET_HTTP_SVR_REQUEST_PARSER_H
#include "net_http_svr_protocol_i.h"

typedef struct net_http_svr_request_parser net_http_svr_request_parser;

#define HTTP_SVR_MAX_MULTIPART_BOUNDARY_LEN 20

struct net_http_svr_request_parser {
    int cs; /* private */
    size_t chunk_size; /* private */
    unsigned eating : 1; /* private */
    net_http_svr_request_t current_request; /* ro */
    const char* header_field_mark;
    const char* header_value_mark;
    const char* query_string_mark;
    const char* path_mark;
    const char* uri_mark;
    const char* fragment_mark;

    net_http_svr_request_t (*new_request)(void*);
    void* data;
};

void net_http_svr_request_parser_init(net_http_svr_request_parser* parser);
size_t net_http_svr_request_parser_execute(net_http_svr_request_parser* parser, const char* data, size_t len);
int net_http_svr_request_parser_has_error(net_http_svr_request_parser* parser);
int net_http_svr_request_parser_is_finished(net_http_svr_request_parser* parser);

#endif
