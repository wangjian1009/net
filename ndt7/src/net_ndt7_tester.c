#include "net_ndt7_tester_i.h"

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

    tester->m_ctx = NULL;
    tester->m_on_complete = NULL;
    tester->m_ctx_free = NULL;

    TAILQ_INSERT_TAIL(&manager->m_testers, tester, m_next);
    
    return tester;
}

void net_ndt7_tester_free(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_ctx_free) {
        tester->m_ctx_free(tester->m_ctx);
        tester->m_ctx_free = NULL;
        tester->m_ctx = NULL;
    }
    tester->m_on_complete = NULL;
    
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
            manager->m_em, "ndt7: %d: can`t start in state %s", tester->m_id, net_ndt7_tester_state_str(tester->m_state));
        return -1;
    }

    return net_ndt7_tester_start_query_names(tester);
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
    case net_ndt7_tester_state_query_names:
        return "query-names";
    case net_ndt7_tester_state_done:
        return "done";
    }
}
