#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_svr_endpoint_i.h"

int net_ssl_svr_endpoint_init(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ssl_svr_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_ssl = SSL_new(driver->m_ssl_ctx);
    if(endpoint->m_ssl == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: svr: endpoint init: create ssl fail");
        return -1;
    }

    BIO * bio = BIO_new(driver->m_bio_method);
    if (bio == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: svr: endpoint init: create bio fail");
        SSL_free(endpoint->m_ssl);
        return -1;
    }
	BIO_set_init(bio, 1);
	BIO_set_data(bio, endpoint);
	BIO_set_shutdown(bio, 0);
    
    endpoint->m_underline =
        net_endpoint_create(
            driver->m_underline_driver, net_schedule_noop_protocol(schedule), NULL);
    if (endpoint->m_underline == NULL) {
        CPE_ERROR(driver->m_em, "net: ssl: svr: endpoint init: create inner driver fail");
        return -1;
    }

    return 0;
}

void net_ssl_svr_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ssl_svr_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_underline) {
        net_endpoint_free(endpoint->m_underline);
        endpoint->m_underline = NULL;
    }

    if (endpoint->m_ssl) {
        SSL_free(endpoint->m_ssl);
        endpoint->m_ssl = NULL;
    }
}

int net_ssl_svr_endpoint_connect(net_endpoint_t base_endpoint) {
    return 0;
}

void net_ssl_svr_endpoint_close(net_endpoint_t base_endpoint) {
}

int net_ssl_svr_endpoint_update(net_endpoint_t base_endpoint) {
    return 0;
}

int net_ssl_svr_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    return 0;
}

int net_ssl_svr_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t * mss) {
    return 0;
}


