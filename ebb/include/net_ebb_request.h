#ifndef NET_EBB_REQUEST_H
#define NET_EBB_REQUEST_H
#include "net_ebb_system.h"

NET_BEGIN_DECL

/* HTTP Methods */
#define EBB_COPY 0x00000001
#define EBB_DELETE 0x00000002
#define EBB_GET 0x00000004
#define EBB_HEAD 0x00000008
#define EBB_LOCK 0x00000010
#define EBB_MKCOL 0x00000020
#define EBB_MOVE 0x00000040
#define EBB_OPTIONS 0x00000080
#define EBB_POST 0x00000100
#define EBB_PROPFIND 0x00000200
#define EBB_PROPPATCH 0x00000400
#define EBB_PUT 0x00000800
#define EBB_TRACE 0x00001000
#define EBB_UNLOCK 0x00002000

/* Transfer Encodings */
#define EBB_IDENTITY 0x00000001
#define EBB_CHUNKED 0x00000002

typedef void (*net_ebb_header_cb)(net_ebb_request_t, const char* at, size_t length, int header_index);
typedef void (*net_ebb_element_cb)(net_ebb_request_t, const char* at, size_t length);

NET_END_DECL

#endif
