#ifndef NET_EBB_MOUNT_POINT_H
#define NET_EBB_MOUNT_POINT_H
#include "net_ebb_system.h"

NET_BEGIN_DECL

net_ebb_mount_point_t net_ebb_mount_point_mount(net_ebb_mount_point_t from, const char * path, void * backend_env, net_ebb_processor_t backend);
int net_ebb_mount_point_unmount(net_ebb_mount_point_t from, const char * path);

net_ebb_mount_point_t net_ebb_mount_point_find_by_path(net_ebb_service_t service, const char * * path);
net_ebb_mount_point_t net_ebb_mount_point_find_child_by_path(net_ebb_mount_point_t mount_point, const char * * path);
net_ebb_mount_point_t net_ebb_mount_point_find_child_by_name(net_ebb_mount_point_t p, const char * name);

NET_END_DECL

#endif
