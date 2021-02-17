#ifndef NET_SMUX_TYPES_H_INCLEDED
#define NET_SMUX_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_smux_session_runing_mode net_smux_session_runing_mode_t;
typedef enum net_smux_session_underline_type net_smux_session_underline_type_t;

typedef struct net_smux_manager * net_smux_manager_t;
typedef struct net_smux_session * net_smux_session_t;
typedef struct net_smux_stream * net_smux_stream_t;

NET_END_DECL

#endif
