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
typedef enum net_ebb_request_transfer_encoding {
    net_ebb_request_transfer_encoding_identity = 0x00000001,
    net_ebb_request_transfer_encoding_chunked = 0x00000002,
} net_ebb_request_transfer_encoding_t;

uint32_t net_ebb_request_method(net_ebb_request_t request);
net_ebb_request_transfer_encoding_t net_ebb_request_transfer_encoding(net_ebb_request_t request);
uint8_t net_ebb_request_expect_continue(net_ebb_request_t request);
uint16_t net_ebb_request_version_major(net_ebb_request_t request);
uint16_t net_ebb_request_version_minor(net_ebb_request_t request);

NET_END_DECL

#endif
