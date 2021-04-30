#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/url.h"
#include "net_endpoint.h"
#include "net_http_req.h"
#include "net_http_endpoint.h"
#include "net_ndt7_tester_i.h"

static void net_ndt7_tester_clear_state_data(net_ndt7_tester_t tester);
static void net_ndt7_tester_set_state(net_ndt7_tester_t tester, net_ndt7_tester_state_t state);

net_ndt7_tester_t
net_ndt7_tester_create(net_ndt7_manage_t manager, net_ndt7_test_type_t test_type) {
    net_ndt7_tester_t tester = mem_alloc(manager->m_alloc, sizeof(struct net_ndt7_tester));
    if (tester == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: tester: alloc fail");
        return NULL;
    }

    tester->m_manager = manager;
    tester->m_id = ++manager->m_idx_max;
    tester->m_type = test_type;
    tester->m_state = net_ndt7_tester_state_init;
    tester->m_target = NULL;

    bzero(&tester->m_state_data, sizeof(tester->m_state_data));

    tester->m_ctx = NULL;
    tester->m_on_complete = NULL;
    tester->m_ctx_free = NULL;

    TAILQ_INSERT_TAIL(&manager->m_testers, tester, m_next);
    
    return tester;
}

void net_ndt7_tester_free(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    net_ndt7_tester_clear_state_data(tester);
    
    if (tester->m_ctx_free) {
        tester->m_ctx_free(tester->m_ctx);
        tester->m_ctx_free = NULL;
        tester->m_ctx = NULL;
    }
    tester->m_on_complete = NULL;

    if (tester->m_target) {
        cpe_url_free(tester->m_target);
        tester->m_target = NULL;
    }

    TAILQ_REMOVE(&manager->m_testers, tester, m_next);
    mem_free(manager->m_alloc, tester);
}

net_ndt7_test_type_t net_ndt7_tester_type(net_ndt7_tester_t tester) {
    return tester->m_type;
}

net_ndt7_tester_state_t net_ndt7_tester_state(net_ndt7_tester_t tester) {
    return tester->m_state;
}

void net_ndt7_tester_set_cb(
    net_ndt7_tester_t tester,
    void * ctx,
    net_ndt7_tester_on_complete_fun_t on_complete,
    void (*ctx_free)(void *))
{
    if (tester->m_ctx_free) {
        tester->m_ctx_free(tester->m_ctx);
    }

    tester->m_ctx = ctx;
    tester->m_ctx_free = ctx_free;
    tester->m_on_complete = on_complete;
}
    
void net_ndt7_tester_clear_cb(net_ndt7_tester_t tester) {
    tester->m_ctx = NULL;
    tester->m_ctx_free = NULL;
    tester->m_on_complete = NULL;
}

int net_ndt7_tester_start(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_state != net_ndt7_tester_state_init) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: can`t start in state %s",
            tester->m_id, net_ndt7_tester_state_str(tester->m_state));
        return -1;
    }

    if (tester->m_target == NULL) {
        net_ndt7_tester_set_state(tester, net_ndt7_tester_state_query_target);
        if (net_ndt7_tester_query_target_start(tester) != 0) return -1;
        return 0;
    }

    return net_ndt7_tester_check_start_next_step(tester);
}

int net_ndt7_tester_check_start_next_step(net_ndt7_tester_t tester) {
    assert(tester->m_target != NULL);

    switch(tester->m_state) {
    case net_ndt7_tester_state_init:
    case net_ndt7_tester_state_query_target:
        switch(tester->m_type) {
        case net_ndt7_test_download:
        case net_ndt7_test_download_and_upload:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_download);
            if (net_ndt7_tester_download_start(tester) != 0) return -1;
            return 0;
        case net_ndt7_test_upload:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_upload);
            if (net_ndt7_tester_upload_start(tester) != 0) return -1;
            return 0;
        }
    case net_ndt7_tester_state_download:
        switch(tester->m_type) {
        case net_ndt7_test_upload:
            assert(0);
            return 0;
        case net_ndt7_test_download:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_download);
            return 0;
        case net_ndt7_test_download_and_upload:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_upload);
            if (net_ndt7_tester_upload_start(tester) != 0) return -1;
            return 0;
        }
    case net_ndt7_tester_state_upload:
        switch(tester->m_type) {
        case net_ndt7_test_upload:
        case net_ndt7_test_download_and_upload:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_download);
            return 0;
        case net_ndt7_test_download:
            assert(0);
            return 0;
        }
    case net_ndt7_tester_state_done:
        assert(0);
        return 0;
    }
}

static void net_ndt7_tester_clear_state_data(net_ndt7_tester_t tester) {
    switch(tester->m_state) {
    case net_ndt7_tester_state_init:
        break;
    case net_ndt7_tester_state_query_target:
        if (tester->m_state_data.m_query_target.m_req) {
            net_http_req_clear_reader(tester->m_state_data.m_query_target.m_req);
            tester->m_state_data.m_query_target.m_req = NULL;
        }

        if (tester->m_state_data.m_query_target.m_endpoint) {
            net_endpoint_set_data_watcher(
                net_http_endpoint_base_endpoint(tester->m_state_data.m_query_target.m_endpoint),
                NULL, NULL, NULL);
            net_http_endpoint_free(tester->m_state_data.m_query_target.m_endpoint);
            tester->m_state_data.m_query_target.m_endpoint = NULL;
        }
        break;
    case net_ndt7_tester_state_download:
        break;
    case net_ndt7_tester_state_upload:
        break;
    case net_ndt7_tester_state_done:
        break;
    }
}

static void net_ndt7_tester_set_state(net_ndt7_tester_t tester, net_ndt7_tester_state_t state) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_state == state) return;

    net_ndt7_tester_clear_state_data(tester);

    CPE_INFO(
        manager->m_em, "ndt: %d: state %s ==> %s",
        tester->m_id, net_ndt7_tester_state_str(tester->m_state), net_ndt7_tester_state_str(state));

    tester->m_state = state;
}

const char * net_ndt7_test_type_str(net_ndt7_test_type_t state) {
    switch(state) {
    case net_ndt7_test_upload:
        return "upload";
    case net_ndt7_test_download:
        return "download";
    case net_ndt7_test_download_and_upload:
        return "download-and-upload";
    }
}

const char * net_ndt7_tester_state_str(net_ndt7_tester_state_t state) {
    switch (state) {
    case net_ndt7_tester_state_init:
        return "init";
    case net_ndt7_tester_state_query_target:
        return "query-target";
    case net_ndt7_tester_state_download:
        return "download";
    case net_ndt7_tester_state_upload:
        return "upload";
    case net_ndt7_tester_state_done:
        return "done";
    }
}
