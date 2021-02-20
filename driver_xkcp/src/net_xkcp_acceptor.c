#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_dgram.h"
#include "net_timer.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_xkcp_acceptor_i.h"
#include "net_xkcp_endpoint_i.h"

static void net_xkcp_acceptor_recv(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);
static void net_xkcp_acceptor_update(net_timer_t timer, void * ctx);

int net_xkcp_acceptor_init(net_acceptor_t base_acceptor) {
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_xkcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    net_address_t address = net_acceptor_address(base_acceptor);
    if (address == NULL) {
        CPE_ERROR(driver->m_em, "xkcp: acceptor: init: no address!");
        return -1;
    }
    
    acceptor->m_dgram = net_dgram_create(driver->m_underline_driver, address, net_xkcp_acceptor_recv, base_acceptor);
    if (acceptor->m_dgram == NULL) {
        CPE_ERROR(driver->m_em, "xkcp: acceptor: init: create dgram fail!");
        return -1;
    }

    acceptor->m_update_timer =
        net_timer_auto_create(
            net_acceptor_schedule(base_acceptor), net_xkcp_acceptor_update, base_acceptor);
    if (acceptor->m_update_timer == NULL) {
        CPE_ERROR(driver->m_em, "xkcp: acceptor: init: create update timer fail!");
        net_dgram_free(acceptor->m_dgram);
        return -1;
    }

    return 0;
}

void net_xkcp_acceptor_fini(net_acceptor_t base_acceptor) {
    net_xkcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    if (acceptor->m_update_timer) {
        net_timer_free(acceptor->m_update_timer);
        acceptor->m_update_timer = NULL;
    }

    if (acceptor->m_dgram) {
        net_dgram_free(acceptor->m_dgram);
        acceptor->m_dgram = NULL;
    }
}

static void net_xkcp_acceptor_recv(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source) {
}

static void net_xkcp_acceptor_update(net_timer_t timer, void * ctx) {
}
