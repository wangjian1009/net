#include "cmocka_all.h"
#include "test_memory.h"
#include "test_net_tl_op.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_schedule.h"
#include "net_ws_endpoint.h"
#include "net_ws_protocol.h"
#include "net_ssl_stream_driver.h"
#include "test_ws_svr_mock_svr.h"

static int test_ws_svr_mock_svr_on_new_endpoint(void * ctx, net_endpoint_t endpoint);

test_ws_svr_mock_svr_t
test_ws_svr_mock_svr_create(test_ws_svr_testenv_t env, const char * name, const char * str_url) {
    test_ws_svr_mock_svr_t svr = mem_alloc(test_allocrator(), sizeof(struct test_ws_svr_mock_svr));
    assert_true(svr);

    svr->m_env = env;
    svr->m_url = cpe_str_mem_dup(test_allocrator(), str_url);
    assert_true(svr->m_url);

    /*协议 */
    svr->m_ws_protocol = net_ws_protocol_create(env->m_schedule, name, test_allocrator(), env->m_em);
    assert_true(svr->m_ws_protocol != NULL);
    net_protocol_set_debug(net_protocol_from_data(svr->m_ws_protocol), 2);

    /*驱动 */
    svr->m_ssl_driver = NULL;
    net_driver_t driver = NULL;

    cpe_url_t url = cpe_url_parse(test_allocrator(), env->m_em, str_url);
    if (strcasecmp(cpe_url_protocol(url), "ws") == 0) {
        driver = net_driver_from_data(env->m_driver);
    }
    else if (strcasecmp(cpe_url_protocol(url), "wss") == 0) {
        net_ssl_stream_driver_t ssl_driver =
            net_ssl_stream_driver_create(
                env->m_schedule, name,
                net_driver_from_data(env->m_driver),
                test_allocrator(),
                env->m_em);
        assert_true(net_ssl_stream_driver_svr_prepaired(ssl_driver) == 0);
        svr->m_ssl_driver = net_driver_from_data(ssl_driver);
        driver = svr->m_ssl_driver;
    }
    else {
        fail_msg("not support protocol %s", cpe_url_protocol(url));
    }
    
    /*listener */
    net_address_t address = net_address_create_from_url(env->m_schedule, url);
    assert_true(address);

    svr->m_acceptor =
        net_acceptor_create(
            driver, net_protocol_from_data(svr->m_ws_protocol), address, 0,
            test_ws_svr_mock_svr_on_new_endpoint, svr);
    assert_true(svr->m_acceptor);
    net_address_free(address);

    cpe_url_free(url);

    TAILQ_INSERT_TAIL(&env->m_svrs, svr, m_next);
    
    return svr;
}

void test_ws_svr_mock_svr_free(test_ws_svr_mock_svr_t svr) {
    test_ws_svr_testenv_t env = svr->m_env;

    if (svr->m_acceptor) {
        net_acceptor_free(svr->m_acceptor);
        svr->m_acceptor = NULL;
    }

    net_ws_protocol_free(svr->m_ws_protocol);
    
    if (svr->m_ssl_driver) {
        net_driver_free(svr->m_ssl_driver);
        svr->m_ssl_driver = NULL;
    }
    
    if (svr->m_url) {
        mem_free(test_allocrator(), svr->m_url);
        svr->m_url = NULL;
    }

    TAILQ_REMOVE(&env->m_svrs, svr, m_next);

    mem_free(test_allocrator(), svr);
}

test_ws_svr_mock_svr_t
test_ws_svr_mock_svr_find(test_ws_svr_testenv_t env, const char * url) {
    test_ws_svr_mock_svr_t svr;

    TAILQ_FOREACH(svr, &env->m_svrs, m_next) {
        if (strcmp(svr->m_url, url) == 0) {
            return svr;
        }
    }

    return NULL;
}

void test_ws_svr_testenv_create_mock_svr(test_ws_svr_testenv_t env, const char * name, const char * url) {
    assert_true(test_ws_svr_mock_svr_create(env, name, url) != NULL);
}

/*回调函数 */
struct test_ws_svr_mock_svr_ctx {
    test_ws_svr_mock_svr_t m_svr;
};

void test_ws_svr_mock_svr_on_connected_cb(void * i_ctx, net_ws_endpoint_t endpoin) {
    struct test_ws_svr_mock_svr_ctx * ctx = i_ctx;
}

void test_ws_svr_mock_svr_on_msg_text_cb(void * i_ctx, net_ws_endpoint_t endpoint, const char * msg) {
    struct test_ws_svr_mock_svr_ctx * ctx = i_ctx;
}

void test_ws_svr_mock_svr_on_msg_bin_cb(void * i_ctx, net_ws_endpoint_t endpoint, const void * msg, uint32_t msg_len) {
    struct test_ws_svr_mock_svr_ctx * ctx = i_ctx;
}

void test_ws_svr_mock_svr_on_close_cb(
    void * i_ctx, net_ws_endpoint_t endpoint, uint16_t status_code, const void * msg, uint32_t msg_len)
{
    struct test_ws_svr_mock_svr_ctx * ctx = i_ctx;
}

static int test_ws_svr_mock_svr_on_new_endpoint(void * ctx, net_endpoint_t base_endpoint) {
    test_ws_svr_mock_svr_t svr = ctx;
    
    net_ws_endpoint_t endpoint = net_ws_endpoint_cast(base_endpoint);
    net_ws_endpoint_set_runing_mode(endpoint, net_ws_endpoint_runing_mode_svr);

    struct test_ws_svr_mock_svr_ctx * ep_ctx =
        mem_buffer_alloc(&svr->m_env->m_driver->m_setup_buffer, sizeof(struct test_ws_svr_mock_svr_ctx));
    ep_ctx->m_svr = svr;

    net_ws_endpoint_set_callback(
        endpoint,
        ep_ctx,
        test_ws_svr_mock_svr_on_connected_cb,
        test_ws_svr_mock_svr_on_msg_text_cb,
        test_ws_svr_mock_svr_on_msg_bin_cb,
        test_ws_svr_mock_svr_on_close_cb,
        NULL);
    
    return 0;
}

void test_ws_svr_mock_svr_on_connected(
    test_ws_svr_mock_svr_t svr, test_net_ws_endpoint_action_t action, int64_t delay_ms)
{
    
}
