#include "assert.h"
#include "event2/bufferevent.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_watcher.h"
#include "net_driver.h"
#include "net_libevent_watcher.h"

void net_libevent_watcher_read_cb(struct bufferevent * bev, void *ptr);
void net_libevent_watcher_write_cb(struct bufferevent * bev, void* ptr);
void net_libevent_watcher_event_cb(struct bufferevent* bev, short events, void* ptr);

int net_libevent_watcher_init(net_watcher_t base_watcher, int fd) {
    /* net_libevent_driver_t driver = net_driver_data(net_watcher_driver(base_watcher)); */
    /* net_libevent_watcher_t watcher = net_watcher_data(base_watcher); */
    /* int options = 0; */
    /* watcher->m_bufferevent = bufferevent_socket_new(driver->m_event_base, fd, options); */
    /* bufferevent_setcb( */
    /*     watcher->m_bufferevent, */
    /*     net_libevent_watcher_read_cb, */
    /*     net_libevent_watcher_write_cb, */
    /*     net_libevent_watcher_event_cb, */
    /*     base_watcher); */
    return 0;
}

void net_libevent_watcher_fini(net_watcher_t base_watcher, int fd) {
    /* net_libevent_watcher_t watcher = net_watcher_data(base_watcher); */
    /* bufferevent_free(watcher->m_bufferevent); */
}

void net_libevent_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write) {
    net_libevent_watcher_t watcher = net_watcher_data(base_watcher);
    net_libevent_driver_t driver = net_driver_data(net_watcher_driver(base_watcher));
/*     ev_io_stop(driver->m_libevent_loop, &watcher->m_watcher); */

/*     int kind = (expect_read ? EV_READ : 0) | (expect_write ? EV_WRITE : 0); */
/*     if (kind) { */
/* #ifdef _WIN32 */
/*         ev_io_init(&watcher->m_watcher, net_libevent_watcher_cb, _open_osfhandle(fd, 0) , kind); */
/* #else */
/*         ev_io_init(&watcher->m_watcher, net_libevent_watcher_cb, fd, kind); */
/* #endif */
/*         ev_io_start(driver->m_libevent_loop, &watcher->m_watcher); */
/*     } */
}

void net_libevent_watcher_read_cb(struct bufferevent *bev, void *ptr) {
    /* http2_session_data* session_data = (http2_session_data*)ptr; */
    /* (void)bev; */

    /* if (session_recv(session_data) != 0) { */
    /*     delete_http2_session_data(session_data); */
    /*     return; */
    /* } */
}

void net_libevent_watcher_write_cb(struct bufferevent* bev, void* ptr) {
    /* http2_session_data* session_data = (http2_session_data*)ptr; */
    /* if (evbuffer_get_length(bufferevent_get_output(bev)) > 0) { */
    /*     return; */
    /* } */
    /* if (nghttp2_session_want_read(session_data->session) == 0 && nghttp2_session_want_write(session_data->session) == 0) { */
    /*     delete_http2_session_data(session_data); */
    /*     return; */
    /* } */
    /* if (session_send(session_data) != 0) { */
    /*     delete_http2_session_data(session_data); */
    /*     return; */
    /* } */
}

void net_libevent_watcher_event_cb(struct bufferevent* bev, short events, void* ptr) {
/*     http2_session_data* session_data = (http2_session_data*)ptr; */
/*     if (events & BEV_EVENT_CONNECTED) { */
/*         const unsigned char* alpn = NULL; */
/*         unsigned int alpnlen = 0; */
/*         SSL* ssl; */
/*         (void)bev; */

/*         fprintf(stderr, "%s connected\n", session_data->client_addr); */

/*         ssl = bufferevent_openssl_get_ssl(session_data->bev); */

/* #ifndef OPENSSL_NO_NEXTPROTONEG */
/*         SSL_get0_next_proto_negotiated(ssl, &alpn, &alpnlen); */
/* #endif /\* !OPENSSL_NO_NEXTPROTONEG *\/ */
/* #if OPENSSL_VERSION_NUMBER >= 0x10002000L */
/*         if (alpn == NULL) { */
/*             SSL_get0_alpn_selected(ssl, &alpn, &alpnlen); */
/*         } */
/* #endif /\* OPENSSL_VERSION_NUMBER >= 0x10002000L *\/ */

/*         if (alpn == NULL || alpnlen != 2 || memcmp("h2", alpn, 2) != 0) { */
/*             fprintf(stderr, "%s h2 is not negotiated\n", session_data->client_addr); */
/*             delete_http2_session_data(session_data); */
/*             return; */
/*         } */

/*         initialize_nghttp2_session(session_data); */

/*         if (send_server_connection_header(session_data) != 0 || session_send(session_data) != 0) { */
/*             delete_http2_session_data(session_data); */
/*             return; */
/*         } */

/*         return; */
/*     } */
/*     if (events & BEV_EVENT_EOF) { */
/*         fprintf(stderr, "%s EOF\n", session_data->client_addr); */
/*     } else if (events & BEV_EVENT_ERROR) { */
/*         fprintf(stderr, "%s network error\n", session_data->client_addr); */
/*     } else if (events & BEV_EVENT_TIMEOUT) { */
/*         fprintf(stderr, "%s timeout\n", session_data->client_addr); */
/*     } */
/*     delete_http2_session_data(session_data); */
}

/* static void net_libevent_watcher_cb(EV_P_ ev_io * iow, int revents) { */
/*     net_libevent_watcher_t watcher = iow->data; */
/*     net_watcher_t base_watcher = net_watcher_from_data(watcher); */

/*     net_watcher_notify(base_watcher, (revents | EV_READ) ? 1 : 0, (revents | EV_WRITE) ? 1 : 0); */
/* } */
