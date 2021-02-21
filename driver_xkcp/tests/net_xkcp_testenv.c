#include "yajl/yajl_tree.h"
#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_pair.h"
#include "net_xkcp_testenv.h"
#include "net_xkcp_config.h"
#include "net_xkcp_acceptor.h"
#include "net_xkcp_driver.h"

net_xkcp_testenv_t net_xkcp_testenv_create() {
    net_xkcp_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_xkcp_testenv));
    env->m_tem = test_error_monitor_create();
    env->m_em = test_error_monitor_em(env->m_tem);
    env->m_schedule = net_schedule_create(test_allocrator(), env->m_em);
    env->m_test_protocol = test_net_protocol_create(env->m_schedule, "test-protocol");
    env->m_tdriver = test_net_driver_create(env->m_schedule, env->m_em);

    const char * addition_name = "test";
    
    env->m_xkcp_driver =
        net_xkcp_driver_create(
            env->m_schedule, addition_name,
            net_driver_from_data(env->m_tdriver),
            test_allocrator(), env->m_em);
    net_driver_set_debug(net_driver_from_data(env->m_xkcp_driver), 2);
    
    return env;
}

void net_xkcp_testenv_free(net_xkcp_testenv_t env) {
    net_schedule_free(env->m_schedule);
    test_error_monitor_free(env->m_tem);
    mem_free(test_allocrator(), env);
}

net_xkcp_connector_t
net_xkcp_testenv_create_connector(
    net_xkcp_testenv_t env, const char * str_address, const char * str_config)
{
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_xkcp_config_t config = NULL;
    struct net_xkcp_config config_buf;
    if (str_config) {
        net_xkcp_endpoint_parse_config(env, &config_buf, str_config);
        config = &config_buf;
    }
    
    net_xkcp_connector_t connector = net_xkcp_connector_create(env->m_xkcp_driver, address, config);
    assert_true(connector);

    return connector;
}

net_xkcp_acceptor_t
net_xkcp_testenv_create_acceptor(
    net_xkcp_testenv_t env, const char * str_address, const char * config)
{
    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);

    net_acceptor_t base_acceptor =
        net_acceptor_create(
            net_driver_from_data(env->m_xkcp_driver),
            env->m_test_protocol, address, 0,
            NULL, NULL);
    assert_true(base_acceptor);
        
    net_address_free(address);

    net_xkcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    if (config) {
        struct net_xkcp_config xkcp_config;
        net_xkcp_endpoint_parse_config(env, &xkcp_config, config);
        assert_true(net_xkcp_acceptor_set_config(acceptor, &xkcp_config) == 0);
    }

    return acceptor;
}

void net_xkcp_endpoint_parse_config(
    net_xkcp_testenv_t env, net_xkcp_config_t config, const char * str_cfg)
{
    net_xkcp_config_init_default(config);

    char error_buf[256];
    yajl_val cfg = yajl_tree_parse(str_cfg, error_buf, sizeof(error_buf));
    if (cfg == NULL) {
        fail_msg("parse json error\n%s", error_buf);
    }

    const char * str_mode = yajl_get_string(yajl_tree_get_2(cfg, "mode", yajl_t_string));
    if (str_mode == NULL) {
        if (net_xkcp_mode_from_str(&config->m_mode, str_mode) != 0) {
            fail_msg("mode %s unknown", str_mode);
        }
    }

    yajl_val mtu = yajl_tree_get_2(cfg, "mtu", yajl_t_string);
    if (mtu) {
        
    }

    yajl_tree_free(cfg);
}

net_xkcp_endpoint_t
net_xkcp_testenv_create_ep(net_xkcp_testenv_t env) {
    net_endpoint_t base_endpoint =
        net_endpoint_create(
            net_driver_from_data(env->m_xkcp_driver),
            env->m_test_protocol,
            NULL);

    net_xkcp_endpoint_t endpoint = net_xkcp_endpoint_cast(base_endpoint);
    assert_true(endpoint);

    return endpoint;
}

net_xkcp_endpoint_t
net_xkcp_testenv_create_cli_ep(net_xkcp_testenv_t env, const char * str_address) {
    net_xkcp_endpoint_t endpoint = net_xkcp_testenv_create_ep(env);

    net_address_t remote_address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(remote_address != NULL);

    assert_true(
        net_endpoint_set_remote_address(net_xkcp_endpoint_base_endpoint(endpoint), remote_address) == 0);
    net_address_free(remote_address);

    return endpoint;
}

