#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_string.h"
#include "net_address.h"
#include "net_watcher.h"
#include "net_icmp_ping_processor_i.h"
#include "net_icmp_pro.h"

static int net_icmp_ping_processor_send(net_icmp_ping_processor_t processor, const char * msg);
static void net_icmp_ping_processor_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
static uint16_t net_icmp_checksum(uint8_t const * buf, uint32_t len);

net_icmp_ping_processor_t
net_icmp_ping_processor_create(net_icmp_ping_task_t task, net_address_t target, uint16_t ping_count) {
    net_icmp_mgr_t mgr = task->m_mgr;
    
    net_icmp_ping_processor_t processor = TAILQ_FIRST(&mgr->m_free_ping_processors);
    if (processor) {
        TAILQ_REMOVE(&mgr->m_free_ping_processors, processor, m_next);
    }
    else {
        processor = mem_alloc(mgr->m_alloc, sizeof(struct net_icmp_ping_processor));
        if (processor == NULL) {
            CPE_ERROR(mgr->m_em, "icmp: ping: %s: processor: alloc fail!", net_address_dump(net_icmp_mgr_tmp_buffer(mgr), target));
            return NULL;
        }
    }

    processor->m_task = task;
    processor->m_ping_id = ++mgr->m_ping_id_max;
    processor->m_ping_index = 0;
    processor->m_ping_count = ping_count;
    processor->m_fd = -1;
    processor->m_watcher = NULL;
    
    processor->m_target = net_address_copy(mgr->m_schedule, target);
    if (processor->m_target == NULL) {
        CPE_ERROR(mgr->m_em, "icmp: ping: %s: processor: dup target address fail!", net_address_dump(net_icmp_mgr_tmp_buffer(mgr), target));
        processor->m_task = (net_icmp_ping_task_t)mgr;
        TAILQ_INSERT_TAIL(&mgr->m_free_ping_processors, processor, m_next);
        return NULL;
    }

    return processor;
}

void net_icmp_ping_processor_free(net_icmp_ping_processor_t processor) {
    net_icmp_mgr_t mgr = processor->m_task->m_mgr;
    
    if (processor->m_watcher) {
        net_watcher_free(processor->m_watcher);
        processor->m_watcher = NULL;
    }

    if (processor->m_fd != -1) {
        cpe_sock_close(processor->m_fd);
        processor->m_fd = -1;
    }
    
    if (processor->m_target) {
        net_address_free(processor->m_target);
        processor->m_target = NULL;
    }
    
    processor->m_task = (net_icmp_ping_task_t)mgr;
    TAILQ_INSERT_TAIL(&mgr->m_free_ping_processors, processor, m_next);
}

void net_icmp_ping_processor_real_free(net_icmp_ping_processor_t processor) {
    net_icmp_mgr_t mgr = (net_icmp_mgr_t)processor->m_task;

    TAILQ_REMOVE(&mgr->m_free_ping_processors, processor, m_next);
    
    mem_free(mgr->m_alloc, processor);
}

int net_icmp_ping_processor_start(net_icmp_ping_processor_t processor) {
    net_icmp_mgr_t mgr = processor->m_task->m_mgr;

    int af = 0;

    switch(net_address_type(processor->m_target)) {
    case net_address_ipv4:
        af = PF_INET;
        break;
    case net_address_ipv6:
    case net_address_domain:
    case net_address_local:
        CPE_ERROR(
            mgr->m_em, "icmp: ping: %s: start: not support address type %s!", 
            net_address_dump(net_icmp_mgr_tmp_buffer(mgr), processor->m_target),
            net_address_type_str(net_address_type(processor->m_target)));
        return -1;
    }
    
    processor->m_fd = socket(af, SOCK_RAW, IPPROTO_ICMP);
    if (processor->m_fd < 0) {
        CPE_ERROR(
            mgr->m_em, "icmp: ping: %s: start: create raw sock fail, error=%d (%s)!", 
            net_address_dump(net_icmp_mgr_tmp_buffer(mgr), processor->m_target),
            errno, strerror(errno));
        return -1;
    }

    processor->m_watcher = net_watcher_create(mgr->m_driver, processor->m_fd, processor, net_icmp_ping_processor_rw_cb);
    if (processor->m_watcher == NULL) {
        CPE_ERROR(
            mgr->m_em, "icmp: ping: %s: start: create watcher fail!",
            net_address_dump(net_icmp_mgr_tmp_buffer(mgr), processor->m_target));
        cpe_sock_close(processor->m_fd);
        processor->m_fd = -1;
        return -1;
    }
    
    if (net_icmp_ping_processor_send(processor, "hello") != 0) {
        net_watcher_free(processor->m_watcher);
        processor->m_watcher = NULL;
        cpe_sock_close(processor->m_fd);
        processor->m_fd = -1;
        return -1;
    }

    net_watcher_update_read(processor->m_watcher, 1);

    return 0;
}

static int net_icmp_ping_processor_send(net_icmp_ping_processor_t processor, const char * msg) {
    net_icmp_mgr_t mgr = processor->m_task->m_mgr;
    
    uint8_t sendbuf[256];
    
    struct net_icmp_hdr * icmp_hdr = (struct net_icmp_hdr *)sendbuf;
    uint16_t len = sizeof(*icmp_hdr);

    bzero(icmp_hdr, len);
    
    icmp_hdr->type = NET_ICMP_ECHO;
    CPE_COPY_HTON16(&icmp_hdr->un.echo.id, &processor->m_ping_id);

    processor->m_ping_index++;
    CPE_COPY_HTON16(&icmp_hdr->un.echo.sequence, &processor->m_ping_index);

    gettimeofday((struct timeval *)(sendbuf + len), NULL);
    len += sizeof(struct timeval);

    uint16_t msg_len = strlen(msg);
    memcpy((sendbuf + len), msg, msg_len);
    len += msg_len;
    
    icmp_hdr->checksum = net_icmp_checksum(sendbuf, len);  //计算校验和 */
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (net_address_to_sockaddr(processor->m_target, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            mgr->m_em, "icmp: ping: %s: send: %d.%d: ==> to socket addr fail",
            net_address_dump(net_icmp_mgr_tmp_buffer(mgr), processor->m_target),
            processor->m_ping_id, processor->m_ping_index);
        return -1;
    }

    if (sendto(processor->m_fd, sendbuf, len, 0, (struct sockaddr *)&addr, addr_len) != 0) {
        CPE_ERROR(
            mgr->m_em, "icmp: ping: %s: send: %d.%d: ==> socket send fail, error=%d (%s)",
            net_address_dump(net_icmp_mgr_tmp_buffer(mgr), processor->m_target),
            processor->m_ping_id, processor->m_ping_index,
            errno, strerror(errno));
        return -1;
    }
    
    CPE_INFO(
        mgr->m_em, "icmp: ping: %s: send: %d.%d: ==> msg=%s, len=%d, checksum=:0x%x", 
        net_address_dump(net_icmp_mgr_tmp_buffer(mgr), processor->m_target),
        processor->m_ping_id, processor->m_ping_index,
        msg, (int)len, icmp_hdr->checksum);

    return 0;
}

static void net_icmp_ping_processor_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    if (do_read) {
        uint8_t recvbuf[256];
        
    }
}

static uint16_t net_icmp_checksum(uint8_t const * buf, uint32_t len) {
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
