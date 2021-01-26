#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_ssl_cli_driver_i.h"
#include "net_ssl_cli_endpoint_i.h"
#include "net_ssl_cli_underline_i.h"
#include "net_ssl_cli_underline_bio.h"

static int net_ssl_cli_driver_init(net_driver_t driver);
static void net_ssl_cli_driver_fini(net_driver_t driver);

net_ssl_cli_driver_t
net_ssl_cli_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-cli-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl-cli");
    }

    net_driver_t base_driver =
        net_driver_create(
            schedule,
            name,
            /*driver*/
            sizeof(struct net_ssl_cli_driver),
            net_ssl_cli_driver_init,
            net_ssl_cli_driver_fini,
            NULL,
            /*timer*/
            0, NULL, NULL, NULL, NULL, NULL,
            /*acceptor*/
            0, NULL, NULL,
            /*endpoint*/
            sizeof(struct net_ssl_cli_endpoint),
            net_ssl_cli_endpoint_init,
            net_ssl_cli_endpoint_fini,
            net_ssl_cli_endpoint_connect,
            net_ssl_cli_endpoint_update,
            net_ssl_cli_endpoint_set_no_delay,
            net_ssl_cli_endpoint_get_mss,
            /*dgram*/
            0, NULL, NULL, NULL,
            /*watcher*/
            0, NULL, NULL, NULL);

    net_ssl_cli_driver_t ssl_driver = net_driver_data(base_driver);

    ssl_driver->m_alloc = alloc;
    ssl_driver->m_em = em;
    ssl_driver->m_underline_driver = underline_driver;
    ssl_driver->m_underline_protocol =
        net_ssl_cli_underline_protocol_create(schedule, name, ssl_driver);

    return ssl_driver;
}

void net_ssl_cli_driver_free(net_ssl_cli_driver_t cli_driver) {
    net_driver_free(net_driver_from_data(cli_driver));
}

net_ssl_cli_driver_t
net_ssl_cli_driver_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-cli-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl-cli");
    }

    net_driver_t driver = net_driver_find(schedule, name);
    return driver ? net_driver_data(driver) : NULL;
}

static int net_ssl_cli_driver_init(net_driver_t base_driver) {
    net_ssl_cli_driver_t driver = net_driver_data(base_driver);
    driver->m_alloc = NULL;
    driver->m_em = NULL;
    driver->m_underline_driver = NULL;
    driver->m_underline_protocol = NULL;

    driver->m_bio_method = BIO_meth_new(58, "net-ssl-cli");
    if (driver->m_bio_method == NULL) {
        CPE_ERROR(
            net_schedule_em(net_driver_schedule(base_driver)),
            "net: ssl: driver: init: create bio method fail");
        return -1;
    }
    BIO_meth_set_write(driver->m_bio_method, net_ssl_cli_underline_bio_write);
    BIO_meth_set_read(driver->m_bio_method, net_ssl_cli_underline_bio_read);
    BIO_meth_set_puts(driver->m_bio_method, net_ssl_cli_underline_bio_puts);
    BIO_meth_set_ctrl(driver->m_bio_method, net_ssl_cli_underline_bio_ctrl);
    BIO_meth_set_create(driver->m_bio_method, net_ssl_cli_underline_bio_new);
    BIO_meth_set_destroy(driver->m_bio_method, net_ssl_cli_underline_bio_free);
    
    driver->m_ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    if(driver->m_ssl_ctx == NULL) {
        CPE_ERROR(
            net_schedule_em(net_driver_schedule(base_driver)),
            "net: ssl: driver: init: create ssl ctx fail");
        BIO_meth_free(driver->m_bio_method);
        return -1;
    }

    SSL_CTX_set_options(
        driver->m_ssl_ctx,
        SSL_OP_ALL
        | SSL_OP_NO_SSLv2
        | SSL_OP_NO_SSLv3
        | SSL_OP_NO_COMPRESSION
        | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

    //SSL_CTX_set_next_proto_select_cb(driver->m_ssl_ctx, sfox_sfox_protocol_select_next_proto_cb, driver);

    SSL_CTX_set_alpn_protos(driver->m_ssl_ctx, (const unsigned char *)"\x02h2", 3);
    
    
    return 0;
}

static void net_ssl_cli_driver_fini(net_driver_t base_driver) {
    net_ssl_cli_driver_t driver = net_driver_data(base_driver);

    if (driver->m_underline_protocol) {
        net_protocol_free(driver->m_underline_protocol);
        driver->m_underline_protocol = NULL;
    }
    
    if (driver->m_bio_method) {
        BIO_meth_free(driver->m_bio_method);
        driver->m_bio_method = NULL;
    }

    if (driver->m_ssl_ctx) {
        SSL_CTX_free(driver->m_ssl_ctx);
        driver->m_ssl_ctx = NULL;
    }
}

mem_buffer_t net_ssl_cli_driver_tmp_buffer(net_ssl_cli_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
