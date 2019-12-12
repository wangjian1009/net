#include "assert.h"
#include "udns.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_stdlib.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_dns_manage.h"
#include "net_dns_source.h"
#include "net_dns_task.h"
#include "net_dns_task_ctx.h"
#include "net_dns_udns_source_ctx_i.h"

uint8_t net_dns_udns_source_ctx_all_query_done(net_dns_udns_source_ctx_t udns_ctx);

int net_dns_udns_source_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    net_dns_udns_source_t udns = net_dns_source_data(source);
    net_dns_udns_source_ctx_t udns_ctx = (net_dns_udns_source_ctx_t)net_dns_task_ctx_data(task_ctx);

    bzero(udns_ctx, sizeof(*udns_ctx));
    
    return 0;
}

void net_dns_udns_source_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    net_dns_udns_source_ctx_cancel(source, task_ctx);
}

void net_dns_udns_source_ctx_query_v4_cb(struct dns_ctx *ctx, struct dns_rr_a4 *result, void *data) {
    net_dns_task_ctx_t task_ctx = data;
    net_dns_udns_source_t udns = net_dns_source_data(net_dns_task_ctx_source(task_ctx));
    net_dns_task_t task = net_dns_task_ctx_task(task_ctx);
    const char * hostname = net_dns_task_hostname_str(task);
    net_dns_udns_source_ctx_t udns_ctx = (net_dns_udns_source_ctx_t)net_dns_task_ctx_data(task_ctx);
    net_schedule_t schedule = net_driver_schedule(udns->m_driver);

    if (result == NULL) {
        if (udns->m_debug) {
            CPE_INFO(udns->m_em, "udns: %s: ipv4 resolv fail: %s", hostname, dns_strerror(dns_status(udns->m_dns_ctx)));
        }
    }
    else {
        if (result->dnsa4_nrr  == 0) {
            if (udns->m_debug) {
                CPE_INFO(udns->m_em, "udns: %s: no ipv4 record", hostname);
            }
        } else {
            assert(result->dnsa4_nrr > 0);
            for (int i = 0; i < result->dnsa4_nrr; i++) {
                net_address_t result_address =
                    net_address_create_ipv4_from_data(schedule, (void *)(result->dnsa4_addr + i), 0u);
                if (result_address == NULL) {
                    CPE_ERROR(udns->m_em, "udns: %s: no ipv4 record", hostname);
                }
            }
        }
    }

    free(result);
    udns_ctx->m_queries[net_dns_udns_source_query_type_ipv4] = NULL; /* mark A query as being completed */

    if (net_dns_udns_source_ctx_all_query_done(udns_ctx)) {
        if (udns_ctx->m_result_count > 0) {
            net_dns_task_ctx_set_success(task_ctx);
        }
        else {
            net_dns_task_ctx_set_empty(task_ctx);
        }
    }
}

void net_dns_udns_source_ctx_query_v6_cb(struct dns_ctx *ctx, struct dns_rr_a6 *result, void *data) {
    net_dns_task_ctx_t task_ctx = data;
    net_dns_udns_source_t udns = net_dns_source_data(net_dns_task_ctx_source(task_ctx));
    net_dns_task_t task = net_dns_task_ctx_task(task_ctx);
    const char * hostname = net_dns_task_hostname_str(task);
    net_dns_udns_source_ctx_t udns_ctx = (net_dns_udns_source_ctx_t)net_dns_task_ctx_data(task_ctx);
    net_schedule_t schedule = net_driver_schedule(udns->m_driver);

    if (result == NULL) {
        if (udns->m_debug) {
            CPE_INFO(udns->m_em, "udns: %s: ipv6 resolv fail: %s", hostname, dns_strerror(dns_status(udns->m_dns_ctx)));
        }
    }
    else {
        if (result->dnsa6_nrr  == 0) {
            if (udns->m_debug) {
                CPE_INFO(udns->m_em, "udns: %s: no ipv6 record", hostname);
            }
        } else {
            assert(result->dnsa6_nrr > 0);
            for (int i = 0; i < result->dnsa6_nrr; i++) {
                net_address_t result_address =
                    net_address_create_ipv6_from_data(schedule, (void *)(result->dnsa6_addr + i), 0u);
                if (result_address == NULL) {
                    CPE_ERROR(udns->m_em, "udns: %s: no ipv6 record", hostname);
                }
            }
        }
    }

    free(result);
    udns_ctx->m_queries[net_dns_udns_source_query_type_ipv6] = NULL; /* mark A query as being completed */

    if (net_dns_udns_source_ctx_all_query_done(udns_ctx)) {
        if (udns_ctx->m_result_count > 0) {
            net_dns_task_ctx_set_success(task_ctx);
        }
        else {
            net_dns_task_ctx_set_empty(task_ctx);
        }
    }
}

int net_dns_udns_source_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    net_dns_udns_source_t udns = net_dns_source_data(source);
    net_dns_udns_source_ctx_t udns_ctx = (net_dns_udns_source_ctx_t)net_dns_task_ctx_data(task_ctx);
    net_dns_task_t task = net_dns_task_ctx_task(task_ctx);
    const char * hostname = net_dns_task_hostname_str(task);

    if (dns_sock(udns->m_dns_ctx) < 0) {
        if (udns->m_debug) {
            CPE_INFO(udns->m_em, "udns: %s: ignore for not open", hostname);
        }
        net_dns_task_ctx_set_empty(task_ctx);
        return 0;
    }

    net_dns_query_type_t query_type = net_dns_task_query_type(task);

    if (query_type == net_dns_query_ipv4 || query_type == net_dns_query_ipv4v6) {
        udns_ctx->m_queries[net_dns_udns_source_query_type_ipv4] =
            dns_submit_a4(udns->m_dns_ctx, hostname, 0, net_dns_udns_source_ctx_query_v4_cb, task_ctx);
        if (udns_ctx->m_queries[net_dns_udns_source_query_type_ipv4] == NULL) {
            CPE_ERROR(udns->m_em, "udns: %s: start ipv4 query fail", hostname);
        }
    }

    if (query_type == net_dns_query_ipv6 || query_type == net_dns_query_ipv4v6) {
        udns_ctx->m_queries[net_dns_udns_source_query_type_ipv6] =
            dns_submit_a6(udns->m_dns_ctx, hostname, 0, net_dns_udns_source_ctx_query_v6_cb, task_ctx);
        if (udns_ctx->m_queries[net_dns_udns_source_query_type_ipv6] == NULL) {
            CPE_ERROR(udns->m_em, "udns: %s: start ipv6 query fail", hostname);
        }
    }

    if (net_dns_udns_source_ctx_all_query_done(udns_ctx)) {
        if (udns->m_debug) {
            CPE_INFO(udns->m_em, "udns: %s: ignore for no query", hostname);
        }
        net_dns_task_ctx_set_empty(task_ctx);
        return 0;
    }
    else {
        TAILQ_INSERT_TAIL(&udns->m_queries, udns_ctx, m_next);
        return 0;
    }
}

void net_dns_udns_source_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx) {
    net_dns_udns_source_t udns = net_dns_source_data(source);
    net_dns_udns_source_ctx_t udns_ctx = (net_dns_udns_source_ctx_t)net_dns_task_ctx_data(task_ctx);

    uint8_t query_count = 0;
    
    for (uint8_t i = 0; i < CPE_ARRAY_SIZE(udns_ctx->m_queries); ++i) {
        if (udns_ctx->m_queries[i] != NULL) {
            query_count++;
            dns_cancel(udns->m_dns_ctx, udns_ctx->m_queries[i]);
            mem_free(udns->m_alloc, udns_ctx->m_queries[i]);
            udns_ctx->m_queries[i] = NULL;
        }
    }

    if (query_count) {
        TAILQ_REMOVE(&udns->m_queries, udns_ctx, m_next);
    }
}

uint8_t net_dns_udns_source_ctx_all_query_done(net_dns_udns_source_ctx_t udns_ctx) {
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(udns_ctx->m_queries); ++i) {
        if (udns_ctx->m_queries[i] != NULL) return 0;
    }

    return 1;
}

void net_dns_udns_source_ctx_active_cancel(net_dns_udns_source_ctx_t udns_ctx) {
    net_dns_task_ctx_t task_ctx = net_dns_task_ctx_from_data(udns_ctx);
    net_dns_udns_source_t udns = net_dns_source_data(net_dns_task_ctx_source(task_ctx));
    
    assert(!net_dns_udns_source_ctx_all_query_done(udns_ctx));

    for (uint8_t i = 0; i < CPE_ARRAY_SIZE(udns_ctx->m_queries); ++i) {
        if (udns_ctx->m_queries[i] != NULL) {
            dns_cancel(udns->m_dns_ctx, udns_ctx->m_queries[i]);
            mem_free(udns->m_alloc, udns_ctx->m_queries[i]);
            udns_ctx->m_queries[i] = NULL;
        }
    }

    TAILQ_REMOVE(&udns->m_queries, udns_ctx, m_next);

    if (udns_ctx->m_result_count > 0) {
        net_dns_task_ctx_set_success(task_ctx);
    } else {
        net_dns_task_ctx_set_empty(task_ctx);
    }
}
