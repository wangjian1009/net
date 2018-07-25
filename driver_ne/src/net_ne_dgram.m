#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_dgram.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_ne_dgram.h"
#include "net_ne_dgram_session.h"

int net_ne_dgram_init(net_dgram_t base_dgram) {
    net_ne_dgram_t dgram = net_dgram_data(base_dgram);
    net_ne_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));

    if (cpe_hash_table_init(
            &dgram->m_sessions,
            driver->m_alloc,
            (cpe_hash_fun_t) net_ne_dgram_session_address_hash,
            (cpe_hash_eq_t) net_ne_dgram_session_address_eq,
            CPE_HASH_OBJ2ENTRY(net_ne_dgram_session, m_hh_for_dgram),
            -1) != 0)
    {
        return -1;
    }

    return 0;
}

void net_ne_dgram_fini(net_dgram_t base_dgram) {
    net_ne_dgram_t dgram = net_dgram_data(base_dgram);

    net_ne_dgram_session_free_all(dgram);
    cpe_hash_table_fini(&dgram->m_sessions);
}

int net_ne_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len) {
    net_ne_dgram_t dgram = net_dgram_data(base_dgram);

    net_ne_dgram_session_t session = net_ne_dgram_session_find(dgram, target);
    if (session == NULL) {
        session = net_ne_dgram_session_create(dgram, target);
        if (session == NULL) {
            return -1;
        }
    }

    return net_ne_dgram_session_send(session, data, data_len);
}

net_ne_driver_t net_ne_dgram_driver(net_ne_dgram_t dgram) {
    return net_driver_data(net_dgram_driver(net_dgram_from_data(dgram)));
}
