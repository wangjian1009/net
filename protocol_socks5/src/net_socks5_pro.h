#ifndef NET_SS_SOCKS5_H_INCLEDED
#define NET_SS_SOCKS5_H_INCLEDED
#include "cpe/pal/pal_platform.h"
#include "net_socks5_types.h"

#define SOCKS4_SVERSION 0x04
#define SOCKS5_SVERSION 0x05

#define SOCKS5_CMD_CONNECT 0x01
#define SOCKS5_CMD_BIND 0x02
#define SOCKS5_CMD_UDP_ASSOCIATE 0x03

#define SOCKS5_ATYPE_IPV4 0x01
#define SOCKS5_ATYPE_DOMAIN 0x03
#define SOCKS5_ATYPE_IPV6 0x04

#define SOCKS5_METHOD_NOAUTH 0x00
#define SOCKS5_METHOD_UNACCEPTABLE 0xff

#define CMD_NOT_SUPPORTED 0x07

CPE_START_PACKED
struct socks5_select_request {
    uint8_t ver;
    uint8_t nmethods;
    uint8_t methods[0];
} CPE_PACKED;

struct socks5_select_response {
    uint8_t ver;
    uint8_t method;
} CPE_PACKED;

struct socks5_connect_request {
    uint8_t ver;
    uint8_t cmd;
    uint8_t rsv;
    uint8_t atyp;
} CPE_PACKED;

struct socks5_connect_response {
    uint8_t ver;
    uint8_t rep;
    uint8_t rsv;
    uint8_t atyp;
} CPE_PACKED;

struct socks4_connect_request {
    uint8_t ver;
    uint8_t cmd;
    uint16_t dst_port;
    uint32_t dst_ip;
    char user_id[];
} CPE_PACKED;

struct socks4_connect_response {
    uint8_t ver;
    uint8_t rep;
    uint16_t dst_port;
    uint32_t dst_ip;
} CPE_PACKED;

CPE_END_PACKED

#endif
