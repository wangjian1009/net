#ifndef NET_HTTP_SVR_MOUNT_POINT_H
#define NET_HTTP_SVR_MOUNT_POINT_H
#include "net_http_svr_system.h"

NET_BEGIN_DECL

net_http_svr_mount_point_t
net_http_svr_mount_point_mount(
    net_http_svr_mount_point_t from, const char * path,
    void * backend_env, net_http_svr_processor_t backend);

int net_http_svr_mount_point_unmount(
    net_http_svr_mount_point_t from, const char * path);

net_http_svr_mount_point_t
net_http_svr_mount_point_find_by_path(net_http_svr_protocol_t service, const char * * path);

net_http_svr_mount_point_t
net_http_svr_mount_point_find_child_by_path(net_http_svr_mount_point_t mount_point, const char * * path);

net_http_svr_mount_point_t
net_http_svr_mount_point_find_child_by_name(net_http_svr_mount_point_t p, const char * name);

NET_END_DECL

#endif
