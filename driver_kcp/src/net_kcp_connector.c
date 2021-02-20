#include <assert.h>
#include "net_address.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_smux_config.h"
#include "net_smux_protocol.h"
#include "net_smux_dgram.h"
#include "net_kcp_connector_i.h"

net_kcp_connector_t
net_kcp_connector_create(
    net_kcp_driver_t driver, net_address_t remote_address, net_kcp_config_t config)
{
    /* net_address_t address = net_acceptor_address(base_acceptor); */
    /* if (address == NULL) { */
    /*     CPE_ERROR(driver->m_em, "net: kcp: acceptor: init: no address!"); */
    /*     return -1; */
    /* } */

    /* struct net_smux_config smux_config = * net_smux_protocol_dft_config(driver->m_smux_protocol); */
    
    /* acceptor->m_smux_dgram = */
    /*     net_smux_dgram_create( */
    /*         driver->m_smux_protocol, */
    /*         net_smux_runing_mode_svr, */
    /*         driver->m_underline_driver, address, &smux_config); */
    /* if (acceptor->m_smux_dgram == NULL) { */
    /*     CPE_ERROR(driver->m_em, "net: kcp: acceptor: init: create mux failed!"); */
    /*     return -1; */
    /* } */
    
    return NULL;
}

void net_kcp_connector_free(net_kcp_connector_t connector) {
}

net_kcp_connector_t
net_kcp_connector_find(net_kcp_driver_t driver, net_address_t remote_address) {
    return NULL;
}
