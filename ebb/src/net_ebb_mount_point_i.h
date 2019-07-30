#ifndef NET_EBB_MOUNT_POINT_I_H
#define NET_EBB_MOUNT_POINT_I_H
#include "net_ebb_mount_point.h"
#include "net_ebb_service_i.h"

struct net_ebb_mount_point {
    net_ebb_service_t m_service;
    char m_name[32];
    size_t m_name_len;

    /*tree*/
    net_ebb_mount_point_t m_parent;
    TAILQ_ENTRY(net_ebb_mount_point) m_next_for_parent;
    net_ebb_mount_point_list_t m_childs;
    
    /*bridger*/
    net_ebb_mount_point_t m_bridger_to;
    TAILQ_ENTRY(net_ebb_mount_point) m_next_bridger_to;
    net_ebb_mount_point_list_t m_bridger_froms;

    /*processor*/
    void * m_processor_env;
    net_ebb_processor_t m_processor;
    TAILQ_ENTRY(net_ebb_mount_point) m_next_for_processor;
};

net_ebb_mount_point_t net_ebb_mount_point_create(
    net_ebb_service_t service, const char * name, const char * name_end, net_ebb_mount_point_t parent, void * processor_env, net_ebb_processor_t processor);
void net_ebb_mount_point_free(net_ebb_mount_point_t mount_point);
void net_ebb_mount_point_real_free(net_ebb_mount_point_t mount_point);
    
net_ebb_mount_point_t net_ebb_mount_point_child_find_by_name_ex(net_ebb_mount_point_t parent, const char * name, const char * name_end);
net_ebb_mount_point_t net_ebb_mount_point_child_find_by_path_ex(net_ebb_mount_point_t parent, const char * path, const char * path_end);

void net_ebb_mount_point_set_processor(net_ebb_mount_point_t mount_point, void * processor_env, net_ebb_processor_t processor);
void net_ebb_mount_point_clear_path(net_ebb_mount_point_t mount_point);

int net_ebb_mount_point_set_bridger_to(net_ebb_mount_point_t mp, net_ebb_mount_point_t to);

#endif
