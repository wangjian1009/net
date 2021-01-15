#include "net_driver.h"
#include "net_schedule.h"
#include "net_ssl_cli_driver_i.h"
#include "net_ssl_cli_endpoint_i.h"

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
            net_ssl_cli_endpoint_close,
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

static int net_ssl_cli_driver_init(net_driver_t driver) {
    net_ssl_cli_driver_t ssl_driver = net_driver_data(driver);
    ssl_driver->m_alloc = NULL;
    ssl_driver->m_em = NULL;
    return 0;
}

static void net_ssl_cli_driver_fini(net_driver_t driver) {
}

