#include <assert.h>
#include "net_endpoint.h"
#include "net_endpoint_monitor.h"
#include "net_driver.h"
#include "net_http2_endpoint.h"
#include "net_http2_stream_using_i.h"
#include "net_http2_stream_endpoint_i.h"

void net_http2_stream_using_on_http2_ep_fini(void * ctx);
void net_http2_stream_using_on_http2_ep_evt(void * ctx, net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt);

net_http2_stream_using_t
net_http2_stream_using_create(
    net_http2_stream_driver_t driver, net_http2_stream_using_list_t * owner, net_http2_endpoint_t http2_ep)
{
    net_http2_stream_using_t using = mem_alloc(driver->m_alloc, sizeof(struct net_http2_stream_using));
    if (using == NULL) return NULL;

    net_endpoint_t base_http2_ep = net_http2_endpoint_base_endpoint(http2_ep);

    using->m_driver = driver;
    using->m_owner = owner;
    using->m_http2_ep = http2_ep;
    TAILQ_INIT(&using->m_streams);

    using->m_monitor =
        net_endpoint_monitor_create(
            base_http2_ep, using,
            net_http2_stream_using_on_http2_ep_fini,
            net_http2_stream_using_on_http2_ep_evt);
    if (using->m_monitor == NULL) {
        mem_free(driver->m_alloc, using);
        return NULL;
    }

    TAILQ_INSERT_TAIL(owner, using, m_next);

    return using;
}

void net_http2_stream_using_free(net_http2_stream_using_t using) {
    net_http2_stream_driver_t driver = using->m_driver;

    if (using->m_monitor) {
        net_endpoint_monitor_free(using->m_monitor);
        using->m_monitor = NULL;
    }

    while(!TAILQ_EMPTY(&using->m_streams)) {
        net_http2_stream_endpoint_set_using(TAILQ_FIRST(&using->m_streams), NULL);
    }
    
    TAILQ_REMOVE(using->m_owner, using, m_next);
    mem_free(driver->m_alloc, using);
}

void net_http2_stream_using_on_http2_ep_fini(void * ctx) {
    net_http2_stream_using_t using = ctx;
    net_http2_stream_endpoint_t stream, next_stream;

    for (stream = TAILQ_FIRST(&using->m_streams); stream; stream = next_stream) {
        next_stream = TAILQ_NEXT(stream, m_next_for_using);
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
        }
    }
}

void net_http2_stream_using_on_http2_ep_evt(void * ctx, net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt) {
    net_http2_stream_using_t using = ctx;
    net_http2_stream_endpoint_t stream, next_stream;
    
    switch(evt->m_type) {
    case net_endpoint_monitor_evt_state_changed:
        for(stream = TAILQ_FIRST(&using->m_streams); stream; stream = next_stream) {
            next_stream = TAILQ_NEXT(stream, m_next_for_using);
            if (net_http2_stream_endpoint_sync_state(stream) != 0) {
                net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
            }
        }
        break;
    case net_endpoint_monitor_evt_write_begin:
        break;
    case net_endpoint_monitor_evt_write_completed:
        break;
    }
}
