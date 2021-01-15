#include <assert.h>
#include "cpe/utils/random.h"
#include "cpe/utils/hex_utils.h"
#include "net_acceptor.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_svr_acceptor_i.h"
#include "net_ssl_svr_endpoint_i.h"

int net_ssl_svr_acceptor_on_new_endpoint(void * ctx, net_endpoint_t endpoint) {
    return 0;
}

int net_ssl_svr_acceptor_init(net_acceptor_t base_acceptor) {
    struct net_ssl_svr_acceptor * acceptor = net_acceptor_data(base_acceptor);
    net_ssl_svr_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    acceptor->m_underline = NULL;

    return 0;
}

void net_ssl_svr_acceptor_fini(net_acceptor_t base_acceptor) {
    struct net_ssl_svr_acceptor * acceptor = net_acceptor_data(base_acceptor);

    if (acceptor->m_underline) {
        net_acceptor_free(acceptor->m_underline);
        acceptor->m_underline = NULL;
    }
}
