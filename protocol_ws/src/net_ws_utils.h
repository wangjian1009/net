#ifndef NET_WS_UTILS_I_H_INCLEDED
#define NET_WS_UTILS_I_H_INCLEDED
#include "wslay/wslay.h"
#include "net_ws_types.h"

NET_BEGIN_DECL

int net_ws_send_http_handshake(wslay_event_context_ptr ctx, net_endpoint_t base_endpoint);

NET_END_DECL

#endif
