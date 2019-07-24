#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_string.h"
#include "net_address.h"
#include "net_watcher.h"
#include "net_ping_processor_i.h"
#include "net_ping_icmp_pro.h"

static int net_ping_processor_icmp_send(net_ping_processor_t processor, const char * msg);
static void net_ping_processor_icmp_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
static uint16_t net_ping_icmp_checksum(uint8_t const * buf, uint32_t len);

int net_ping_processor_start_icmp(net_ping_processor_t processor) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = task->m_mgr;

    int af = 0;

    switch(net_address_type(task->m_target)) {
    case net_address_ipv4:
        af = PF_INET;
        break;
    case net_address_ipv6:
    case net_address_domain:
    case net_address_local:
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: not support address type %s!", 
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            net_address_type_str(net_address_type(task->m_target)));
        return -1;
    }
    
    processor->m_icmp.m_fd = socket(af, SOCK_RAW, IPPROTO_ICMP);
    if (processor->m_icmp.m_fd < 0) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: create raw sock fail, error=%d (%s)!", 
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            errno, strerror(errno));
        return -1;
    }

    processor->m_icmp.m_watcher = net_watcher_create(mgr->m_driver, processor->m_icmp.m_fd, processor, net_ping_processor_icmp_rw_cb);
    if (processor->m_icmp.m_watcher == NULL) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: create watcher fail!",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
        cpe_sock_close(processor->m_icmp.m_fd);
        processor->m_icmp.m_fd = -1;
        return -1;
    }
    
    if (net_ping_processor_icmp_send(processor, "hello") != 0) {
        net_watcher_free(processor->m_icmp.m_watcher);
        processor->m_icmp.m_watcher = NULL;
        cpe_sock_close(processor->m_icmp.m_fd);
        processor->m_icmp.m_fd = -1;
        return -1;
    }

    net_watcher_update_read(processor->m_icmp.m_watcher, 1);

    return 0;
}

static int net_ping_processor_icmp_send(net_ping_processor_t processor, const char * msg) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = task->m_mgr;
    
    uint8_t sendbuf[256];
    
    struct net_icmp_hdr * icmp_hdr = (struct net_icmp_hdr *)sendbuf;
    uint16_t len = sizeof(*icmp_hdr);

    bzero(icmp_hdr, len);
    
    icmp_hdr->type = NET_ICMP_ECHO;
    CPE_COPY_HTON16(&icmp_hdr->un.echo.id, &processor->m_icmp.m_ping_id);

    uint16_t ping_index = (uint16_t)task->m_record_count;
    CPE_COPY_HTON16(&icmp_hdr->un.echo.sequence, &ping_index);

    gettimeofday((struct timeval *)(sendbuf + len), NULL);
    len += sizeof(struct timeval);

    uint16_t msg_len = strlen(msg);
    memcpy((sendbuf + len), msg, msg_len);
    len += msg_len;
    
    icmp_hdr->checksum = net_ping_icmp_checksum(sendbuf, len);  //计算校验和 */
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (net_address_to_sockaddr(task->m_target, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: send: %d.%d: ==> to socket addr fail",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            processor->m_icmp.m_ping_id, ping_index);
        return -1;
    }

    if (sendto(processor->m_icmp.m_fd, sendbuf, len, 0, (struct sockaddr *)&addr, addr_len) != 0) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: send: %d.%d: ==> socket send fail, error=%d (%s)",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            processor->m_icmp.m_ping_id, ping_index,
            errno, strerror(errno));
        return -1;
    }
    
    CPE_INFO(
        mgr->m_em, "ping: %s: send: %d.%d: ==> msg=%s, len=%d, checksum=:0x%x", 
        net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
        processor->m_icmp.m_ping_id, ping_index,
        msg, (int)len, icmp_hdr->checksum);

    return 0;
}

static void net_ping_processor_icmp_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    if (do_read) {
        //uint8_t recvbuf[256];
    }
}

static uint16_t net_ping_icmp_checksum(uint8_t const * buf, uint32_t len) {
    uint32_t sum = 0;
    uint16_t const * cbuf;
    
    cbuf = (uint16_t const *)buf;
    
    while(len > 1) {
        sum += *cbuf++;
        len -= 2;
    }
    
    if(len) {
        sum += *(uint8_t const *)cbuf;
    }
    
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    
    return ~sum;
}
