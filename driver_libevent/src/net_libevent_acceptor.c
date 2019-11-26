#include <event2/listener.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/file.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_driver.h"
#include "net_address.h"
#include "net_libevent_acceptor_i.h"
#include "net_libevent_endpoint.h"

int net_libevent_acceptor_init(net_acceptor_t base_acceptor) {
    return 0;
}

void net_libevent_acceptor_fini(net_acceptor_t base_acceptor) {
}

void net_libevent_accept_cb(struct evconnlistener* listener, int fd, struct sockaddr* addr, int addrlen, void* arg) {
    /* app_context* app_ctx = (app_context*)arg; */
    /* http2_session_data* session_data; */
    /* (void)listener; */

    /* session_data = create_http2_session_data(app_ctx, fd, addr, addrlen); */

    /* bufferevent_setcb(session_data->bev, readcb, writecb, eventcb, session_data); */
}
