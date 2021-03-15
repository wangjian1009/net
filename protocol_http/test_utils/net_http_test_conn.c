#include "cmocka_all.h"
#include "test_memory.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_http_endpoint.h"
#include "net_http_test_conn.h"

net_http_test_conn_t
net_http_test_conn_create(
    net_http_test_protocol_t protocol, net_driver_t driver, net_address_t address)
{
    net_schedule_t schedule = net_driver_schedule(driver);
    
    net_http_test_conn_t conn = mem_alloc(test_allocrator(), sizeof(struct net_http_test_conn));
    conn->m_protocol = protocol;

    conn->m_endpoint = net_http_test_protocol_create_ep(protocol, driver);
    assert_true(conn->m_endpoint);

    net_endpoint_set_remote_address(net_http_endpoint_base_endpoint(conn->m_endpoint), address);

    TAILQ_INSERT_TAIL(&protocol->m_conns, conn, m_next);

    return conn;
}

void net_http_test_conn_free(net_http_test_conn_t conn) {
    net_http_test_protocol_t protocol = conn->m_protocol;
    
    if (conn->m_endpoint) {
        net_http_endpoint_free(conn->m_endpoint);
        conn->m_endpoint = NULL;
    }

    TAILQ_REMOVE(&protocol->m_conns, conn, m_next);

    mem_free(test_allocrator(), conn);
}
