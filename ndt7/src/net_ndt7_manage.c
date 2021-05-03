#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_ssl_protocol.h"
#include "net_ssl_stream_driver.h"
#include "net_ws_protocol.h"
#include "net_http_protocol.h"
#include "net_ndt7_manage_i.h"
#include "net_ndt7_tester_i.h"

static void net_ndt7_manage_delay_process(net_timer_t timer, void * ctx);

net_ndt7_manage_t net_ndt7_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver)
{
    net_ndt7_manage_t manage = mem_alloc(alloc, sizeof(struct net_ndt7_manage));
    if (manage == NULL) {
        CPE_ERROR(em, "ndt7: manage alloc fail");
        return NULL;
    }
    
    bzero(manage, sizeof(*manage));

    manage->m_alloc = alloc;
    manage->m_em = em;
    manage->m_debug = 0;
    manage->m_schedule = schedule;
    manage->m_base_driver = driver;
    manage->m_ssl_driver = NULL;
    manage->m_http_protocol = NULL;
    manage->m_idx_max = 0;

    manage->m_delay_process = net_timer_auto_create(schedule, net_ndt7_manage_delay_process, manage);
    if (manage->m_delay_process == NULL) {
        CPE_ERROR(em, "ndt7: create ssl driver fail");
        mem_free(alloc, manage);
        return NULL;
    }

    net_ssl_stream_driver_t ssl_driver =
        net_ssl_stream_driver_create(manage->m_schedule, "ndt7", driver, alloc, em);
    if (ssl_driver == NULL) {
        CPE_ERROR(em, "ndt7: create ssl driver fail");
        net_timer_free(manage->m_delay_process);
        mem_free(alloc, manage);
        return NULL;
    }
    manage->m_ssl_driver = net_driver_from_data(ssl_driver);

    /* net_ssl_protocol_t ssl_protocol = */
    /*     net_ssl_stream_driver_underline_protocol(ssl_driver); */
    /* net_ssl_protocol_set_ciphersuites_all(ssl_protocol); */

    manage->m_ws_protocol = net_ws_protocol_create(schedule, "ndt7", alloc, em);
    if (manage->m_ws_protocol == NULL) {
        CPE_ERROR(em, "ndt7: create ws protocol fail");
        net_driver_free(manage->m_ssl_driver);
        net_timer_free(manage->m_delay_process);
        mem_free(alloc, manage);
        return NULL;
    }
    
    manage->m_http_protocol = net_http_protocol_create(schedule, "ndt7");
    if (manage->m_http_protocol == NULL) {
        CPE_ERROR(em, "ndt7: create http protocol fail");
        net_ws_protocol_free(manage->m_ws_protocol);
        net_driver_free(manage->m_ssl_driver);
        net_timer_free(manage->m_delay_process);
        mem_free(alloc, manage);
        return NULL;
    }

    TAILQ_INIT(&manage->m_testers);
    TAILQ_INIT(&manage->m_to_notify_testers);
    
    return manage;
}

void net_ndt7_manage_free(net_ndt7_manage_t manage) {
    while(!TAILQ_EMPTY(&manage->m_testers)) {
        net_ndt7_tester_free(TAILQ_FIRST(&manage->m_testers));
    }
    assert(TAILQ_EMPTY(&manage->m_to_notify_testers));

    if (manage->m_delay_process) {
        net_timer_free(manage->m_delay_process);
        manage->m_delay_process = NULL;
    }

    if (manage->m_ws_protocol) {
        net_ws_protocol_free(manage->m_ws_protocol);
        manage->m_ws_protocol = NULL;
    }
    
    if (manage->m_ssl_driver) {
        net_driver_free(manage->m_ssl_driver);
        manage->m_ssl_driver = NULL;
    }

    if (manage->m_http_protocol) {
        net_http_protocol_free(manage->m_http_protocol);
        manage->m_http_protocol = NULL;
    }
    
    mem_free(manage->m_alloc, manage);
}

uint8_t net_ndt7_manage_debug(net_ndt7_manage_t manage) {
    return manage->m_debug;
}

void net_ndt7_manage_set_debug(net_ndt7_manage_t manage, uint8_t debug) {
    manage->m_debug = debug;
}

static void net_ndt7_manage_delay_process(net_timer_t timer, void * ctx) {
    net_ndt7_manage_t manage = ctx;

    while(!TAILQ_EMPTY(&manage->m_to_notify_testers)) {
        net_ndt7_tester_t tester =TAILQ_FIRST(&manage->m_to_notify_testers);
        assert(tester->m_is_to_notify);

        tester->m_is_to_notify = 0;
        TAILQ_REMOVE(&manage->m_to_notify_testers, tester, m_next_for_notify);

        net_ndt7_tester_notify_complete(tester);
    }
}
