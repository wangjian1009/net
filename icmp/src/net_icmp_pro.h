#ifndef NET_ICMP_PROTOCOL_I_H_INCLEDED
#define NET_ICMP_PROTOCOL_I_H_INCLEDED
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_types.h"

/*ICMP消息头部 */
struct net_icmp_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;
        uint32_t gateway;
        struct {
            uint16_t unused;
            uint16_t mtu;
        } frag; /* pmtu发现 */
    } un;
    uint32_t icmp_timestamp[2]; /* 时间戳 */
    uint8_t data[0]; /*ICMP数据占位符 */

#define net_icmp_id un.echo.id
#define net_icmp_seq un.echo.sequence
};

struct net_ip_hdr {
#if defined CPE_LITTLE_ENDIAN
    uint8_t hlen: 4, ver: 4;
#elif defined CPE_BIG_ENDIAN
    uint8_t ver: 4, hlen: 4;
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};

#define NET_ICMP_ECHOREPLY 0 /*Echo应答 */
#define NET_ICMP_ECHO 8 /*Echo请求 */

#endif
