#ifndef NET_SMUX_SESSION_H_INCLEDED
#define NET_SMUX_SESSION_H_INCLEDED
#include "net_smux_types.h"

NET_BEGIN_DECL

enum net_smux_session_runing_mode {
    net_smux_session_runing_mode_cli,
    net_smux_session_runing_mode_svr,
};

net_smux_session_t net_smux_session_create(net_smux_manager_t manager, net_smux_session_runing_mode_t runing_mode);
void net_smux_session_free(net_smux_session_t session);

net_smux_session_runing_mode_t net_smux_session_runing_mode(net_smux_session_t session);

const char * net_smux_session_runing_mode_str(net_smux_session_runing_mode_t runing_mode);

NET_END_DECL

#endif
