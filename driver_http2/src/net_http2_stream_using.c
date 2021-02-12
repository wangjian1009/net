#include "net_endpoint.h"
#include "net_driver.h"
#include "net_http2_stream_using_i.h"
#include "net_http2_stream_endpoint_i.h"

net_http2_stream_using_t
net_http2_stream_using_create(
    net_http2_stream_using_list_t * owner, net_http2_stream_endpoint_t stream_ep, net_http2_endpoint_t http2_ep)
{
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream_ep->m_base_endpoint));
    
    net_http2_stream_using_t using = mem_alloc(driver->m_alloc, sizeof(struct net_http2_stream_using));

    using->m_owner = owner;
    using->m_http2_ep = http2_ep;
    using->m_stream_ep = stream_ep;

    TAILQ_INSERT_TAIL(owner, using, m_next);

    return using;
}

void net_http2_stream_using_free(net_http2_stream_using_t using) {
    net_http2_stream_driver_t driver =
        net_driver_data(net_endpoint_driver(using->m_stream_ep->m_base_endpoint));

    TAILQ_INSERT_TAIL(using->m_owner, using, m_next);
    mem_free(driver->m_alloc, using);
}

