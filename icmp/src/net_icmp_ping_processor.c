#include "cpe/pal/pal_socket.h"
#include "net_address.h"
#include "net_icmp_ping_processor_i.h"
#include "net_icmp_pro.h"

static void net_icmp_ping_processor_send(net_icmp_ping_processor_t processor);

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
            CPE_ERROR(mgr->m_em, "icmp: ping: processor: alloc fail!");
            return NULL;
        }
    }

    processor->m_task = task;
    processor->m_ping_count = ping_count;
    processor->m_fd = -1;

    processor->m_target = net_address_copy(mgr->m_schedule, target);
    if (processor->m_target == NULL) {
        CPE_ERROR(mgr->m_em, "icmp: ping: processor: dup target address fail!");
        processor->m_task = (net_icmp_ping_task_t)mgr;
        TAILQ_INSERT_TAIL(&mgr->m_free_ping_processors, processor, m_next);
        return NULL;
    }

    return processor;
}

void net_icmp_ping_processor_free(net_icmp_ping_processor_t processor) {
    net_icmp_mgr_t mgr = processor->m_task->m_mgr;
    
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

static void net_icmp_ping_processor_send(net_icmp_ping_processor_t processor) {
    
    //struct ip *ip_hdr;   //iphdr为IP头部结构体
    int len;
    int len1;

    char sendbuf[256];
    
    /*icmphdr为ICMP头部结构体 */
    struct net_icmp_hdr * icmp_hdr = (struct net_icmp_hdr *)sendbuf;

    icmp_hdr->type = NET_ICMP_ECHO;//初始化ICMP消息类型type
    icmp_hdr->code = 0;    //初始化消息代码code
    /* icmp_hdr->un.echo.id = pid;   //把进程标识码初始给icmp_id */
    /* icmp_hdr->un.echo.sequence = nsent++;  //发送的ICMP消息序号赋值给icmp序号 */
    
    
    /* icmp_data=(struct icmp_filter*)(sendbuf+ICMP_HSIZE); */
    /* gettimeofday((struct timeval *)icmp_data,NULL); // 获取当前时间 */
    /* icmp_data=(struct icmp_filter*)(sendbuf+ICMP_HSIZE+sizeof(timeval)); //真正数据地方 */
    /* memcpy(icmp_data, hello, strlen(hello)); */
    /* icmp_data=(struct icmp_filter*)(sendbuf+ICMP_HSIZE); //恢复数据指针 */
    /* len=ICMP_HSIZE+sizeof(icmp_filter)+strlen(hello); */
    /* icmp_hdr->checksum=0;    //初始化 */
    /* icmp_hdr->checksum=checksum((u8 *)icmp_hdr,len);  //计算校验和 */
    
    /* printf("The send pack checksum is:0x%x\n",icmp_hdr->checksum); */
    /* sendto(sockfd,sendbuf,len,0,(struct sockaddr *)&dest,sizeof (dest)); //经socket传送数据 */
}
