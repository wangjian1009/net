#ifndef NET_HTTP_SVR_MOUNT_POINT_I_H
#define NET_HTTP_SVR_MOUNT_POINT_I_H
#include "net_http_svr_mount_point.h"
#include "net_http_svr_protocol_i.h"

struct net_http_svr_mount_point {
    net_http_svr_protocol_t m_service;
    char m_name[32];
    size_t m_name_len;

    /*tree*/
    net_http_svr_mount_point_t m_parent;
    TAILQ_ENTRY(net_http_svr_mount_point) m_next_for_parent;
    net_http_svr_mount_point_list_t m_childs;
    
    /*bridger*/
    net_http_svr_mount_point_t m_bridger_to;
    TAILQ_ENTRY(net_http_svr_mount_point) m_next_bridger_to;
    net_http_svr_mount_point_list_t m_bridger_froms;

    /*processor*/
    void * m_processor_env;
    net_http_svr_processor_t m_processor;
    TAILQ_ENTRY(net_http_svr_mount_point) m_next_for_processor;
};

net_http_svr_mount_point_t net_http_svr_mount_point_create(
    net_http_svr_protocol_t service, const char * name, const char * name_end, net_http_svr_mount_point_t parent, void * processor_env, net_http_svr_processor_t processor);
void net_http_svr_mount_point_free(net_http_svr_mount_point_t mount_point);
void net_http_svr_mount_point_real_free(net_http_svr_mount_point_t mount_point);
    
net_http_svr_mount_point_t net_http_svr_mount_point_child_find_by_name_ex(net_http_svr_mount_point_t parent, const char * name, const char * name_end);
net_http_svr_mount_point_t net_http_svr_mount_point_child_find_by_path_ex(net_http_svr_mount_point_t parent, const char * path, const char * path_end);

void net_http_svr_mount_point_set_processor(net_http_svr_mount_point_t mount_point, void * processor_env, net_http_svr_processor_t processor);
void net_http_svr_mount_point_clear_path(net_http_svr_mount_point_t mount_point);

int net_http_svr_mount_point_set_bridger_to(net_http_svr_mount_point_t mp, net_http_svr_mount_point_t to);

#endif
