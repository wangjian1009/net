#include "cmocka_all.h"
#include "test_memory.h"
#include "test_net_tl_op.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "net_protocol.h"
#include "net_http_svr_protocol.h"
#include "net_http_svr_processor.h"
#include "net_http_svr_mount_point.h"
#include "net_ssl_stream_driver.h"
#include "net_http_svr_request.h"
#include "net_http_svr_response.h"
#include "test_http_svr_mock_request.h"
#include "test_http_svr_mock_svr.h"

static void test_http_svr_mock_svr_request_process(void * ctx, net_http_svr_request_t request);

test_http_svr_mock_svr_t
test_http_svr_mock_svr_create(test_http_svr_testenv_t env, const char * name, const char * str_url) {
    test_http_svr_mock_svr_t svr = mem_alloc(test_allocrator(), sizeof(struct test_http_svr_mock_svr));
    assert_true(svr);

    svr->m_env = env;
    svr->m_url = cpe_str_mem_dup(test_allocrator(), str_url);
    assert_true(svr->m_url);

    char full_name[128];
    snprintf(full_name, sizeof(full_name), "ext-%s", name);
    
    /*协议 */
    svr->m_http_protocol = net_http_svr_protocol_create(env->m_schedule, full_name);
    assert_true(svr->m_http_protocol != NULL);
    net_protocol_set_debug(net_http_svr_protocol_to_protocol(svr->m_http_protocol), 2);

    svr->m_http_processor =
        net_http_svr_processor_create(
            svr->m_http_protocol, "test",
            svr,
            /*env*/
            NULL,
            /*request*/
            0, NULL, NULL, NULL,
            test_http_svr_mock_svr_request_process);

    net_http_svr_mount_point_t mount_point =
        net_http_svr_mount_point_mount(
            net_http_svr_protocol_root(svr->m_http_protocol),
            "",
            NULL,
            svr->m_http_processor);

    /*驱动 */
    svr->m_ssl_driver = NULL;
    net_driver_t driver = NULL;

    cpe_url_t url = cpe_url_parse(test_allocrator(), env->m_em, str_url);
    if (strcasecmp(cpe_url_protocol(url), "http") == 0) {
        driver = net_driver_from_data(env->m_driver);
    }
    else if (strcasecmp(cpe_url_protocol(url), "https") == 0) {
        net_ssl_stream_driver_t ssl_driver =
            net_ssl_stream_driver_create(
                env->m_schedule, full_name,
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
            driver, net_protocol_from_data(svr->m_http_protocol), address, 0, NULL, NULL);
    assert_true(svr->m_acceptor);
    net_address_free(address);

    cpe_url_free(url);

    TAILQ_INSERT_TAIL(&env->m_svrs, svr, m_next);
    
    return svr;
}

void test_http_svr_mock_svr_free(test_http_svr_mock_svr_t svr) {
    test_http_svr_testenv_t env = svr->m_env;

    if (svr->m_acceptor) {
        net_acceptor_free(svr->m_acceptor);
        svr->m_acceptor = NULL;
    }

    net_http_svr_processor_free(svr->m_http_processor);
    net_http_svr_protocol_free(svr->m_http_protocol);
    
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

test_http_svr_mock_svr_t
test_http_svr_mock_svr_find(test_http_svr_testenv_t env, const char * url) {
    test_http_svr_mock_svr_t svr;

    TAILQ_FOREACH(svr, &env->m_svrs, m_next) {
        if (strcmp(svr->m_url, url) == 0) {
            return svr;
        }
    }

    return NULL;
}

void test_http_svr_testenv_create_mock_svr(test_http_svr_testenv_t env, const char * name, const char * url) {
    assert_true(test_http_svr_mock_svr_create(env, name, url) != NULL);
}

void test_http_svr_testenv_expect_response(
    test_http_svr_testenv_t env, const char * i_url, const char * i_path,
    int code, const char * code_msg, const char * response,
    uint32_t delay_ms)
{
    test_http_svr_mock_svr_t svr = test_http_svr_mock_svr_find(env, i_url);
    if (svr == NULL) fail_msg("mock svr %s not exist", i_url);

    const char * url = mem_buffer_strdup(&env->m_driver->m_setup_buffer, i_url);
    const char * path = mem_buffer_strdup(&env->m_driver->m_setup_buffer, i_path);
    
    expect_string(test_http_svr_mock_svr_request_process, url, url);
    expect_string(test_http_svr_mock_svr_request_process, path, path);
    will_return(
        test_http_svr_mock_svr_request_process, 
        test_http_svr_request_mock_expect_response(env->m_driver, code, code_msg, response, delay_ms));
}

void test_http_svr_testenv_expect_response_close(
    test_http_svr_testenv_t env, const char * i_url, const char * i_path,
    int code, const char * code_msg, const char * response,
    uint32_t delay_ms)
{
    test_http_svr_mock_svr_t svr = test_http_svr_mock_svr_find(env, i_url);
    assert_true(svr);

    const char * url = mem_buffer_strdup(&env->m_driver->m_setup_buffer, i_url);
    const char * path = mem_buffer_strdup(&env->m_driver->m_setup_buffer, i_path);
    
    expect_string(test_http_svr_mock_svr_request_process, url, url);
    expect_string(test_http_svr_mock_svr_request_process, path, path);
    will_return(
        test_http_svr_mock_svr_request_process, 
        test_http_svr_request_mock_expect_response_close(env->m_driver, code, code_msg, response, delay_ms));
}

void test_http_svr_mock_svr_request_process(void * ctx, net_http_svr_request_t request) {
    test_http_svr_mock_svr_t external_svr = ctx;
    const char * url = external_svr->m_url;
    const char * path = net_http_svr_request_full_path(request);
    
    check_expected(url);
    check_expected(path);
    test_http_svr_mock_request_policy_t policy = mock_type(test_http_svr_mock_request_policy_t);

    test_http_svr_request_mock_apply(external_svr->m_env->m_driver, request, policy);
}
