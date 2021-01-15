#include <assert.h>
#include "cpe/utils/random.h"
#include "cpe/utils/hex_utils.h"
#include "net_acceptor.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ssl_svr_acceptor_i.h"
#include "net_ssl_svr_endpoint_i.h"

int net_ssl_svr_acceptor_init(net_acceptor_t base_acceptor) {
    struct net_ssl_svr_acceptor * acceptor = net_acceptor_data(base_acceptor);
    acceptor->m_acceptor = NULL;
    return 0;
}

void net_ssl_svr_acceptor_fini(net_acceptor_t base_acceptor) {
    struct net_ssl_svr_acceptor * acceptor = net_acceptor_data(base_acceptor);

    if (acceptor->m_acceptor) {
        net_acceptor_free(acceptor->m_acceptor);
        acceptor->m_acceptor = NULL;
    }
}
