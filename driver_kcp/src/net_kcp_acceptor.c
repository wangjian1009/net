#include <assert.h>
#include "net_address.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_smux_session.h"
#include "net_kcp_acceptor_i.h"
#include "net_kcp_endpoint_i.h"

int net_kcp_acceptor_init(net_acceptor_t base_acceptor) {
    net_kcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_kcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    net_address_t address = net_acceptor_address(base_acceptor);
    if (address == NULL) {
        CPE_ERROR(driver->m_em, "net: kcp: acceptor: init: no address!");
        return -1;
    }

    acceptor->m_smux_session = net_smux_session_create(driver->m_smux_manager);
    if (acceptor->m_smux_session == NULL) {
        CPE_ERROR(driver->m_em, "net: kcp: acceptor: init: create mux failed!");
        return -1;
    }
    
    return 0;
}

void net_kcp_acceptor_fini(net_acceptor_t base_acceptor) {
    net_kcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_kcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    if (acceptor->m_smux_session) {
        net_smux_session_free(acceptor->m_smux_session);
        acceptor->m_smux_session = NULL;
    }
}
