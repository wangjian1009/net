#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_xkcp_acceptor_i.h"
#include "net_xkcp_endpoint_i.h"

int net_xkcp_acceptor_init(net_acceptor_t base_acceptor) {
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_xkcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    return 0;
}

void net_xkcp_acceptor_fini(net_acceptor_t base_acceptor) {
    net_xkcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
}

