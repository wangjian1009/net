#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/url.h"
#include "cpe/utils/string_utils.h"
#include "net_timer.h"
#include "net_endpoint.h"
#include "net_http_req.h"
#include "net_http_endpoint.h"
#include "net_ndt7_tester_i.h"
#include "net_ndt7_tester_target_i.h"

static void net_ndt7_tester_clear_state_data(net_ndt7_tester_t tester);
static void net_ndt7_tester_set_state(net_ndt7_tester_t tester, net_ndt7_tester_state_t state);

net_ndt7_tester_t
net_ndt7_tester_create(net_ndt7_manage_t manager) {
    net_ndt7_tester_t tester = mem_alloc(manager->m_alloc, sizeof(struct net_ndt7_tester));
    if (tester == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: tester: alloc fail");
        return NULL;
    }

    tester->m_manager = manager;
    tester->m_id = ++manager->m_idx_max;
    tester->m_type = net_ndt7_test_download_and_upload;
    tester->m_state = net_ndt7_tester_state_init;
    tester->m_is_processing = 0;
    tester->m_is_free = 0;

    tester->m_is_to_notify = 0;

    bzero(&tester->m_state_data, sizeof(tester->m_state_data));

    tester->m_error.m_state = net_ndt7_tester_state_init;
    tester->m_error.m_error = net_ndt7_tester_error_none;
    tester->m_error.m_msg = NULL;

    tester->m_ctx = NULL;
    tester->m_on_complete = NULL;
    tester->m_ctx_free = NULL;

    tester->m_target = NULL;
    TAILQ_INIT(&tester->m_targets);

    TAILQ_INSERT_TAIL(&manager->m_testers, tester, m_next);
    
    return tester;
}

void net_ndt7_tester_free(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_is_processing) {
        tester->m_is_free = 1;
        return;
    }

    while(!TAILQ_EMPTY(&tester->m_targets)) {
        net_ndt7_tester_target_free(TAILQ_FIRST(&tester->m_targets));
    }
    
    net_ndt7_tester_clear_state_data(tester);
    
    if (tester->m_ctx_free) {
        tester->m_ctx_free(tester->m_ctx);
        tester->m_ctx_free = NULL;
        tester->m_ctx = NULL;
    }
    tester->m_on_complete = NULL;

    if (tester->m_error.m_msg) {
        mem_free(manager->m_alloc, tester->m_error.m_msg);
        tester->m_error.m_msg = NULL;
    }

    if (tester->m_is_to_notify) {
        tester->m_is_to_notify = 1;
        TAILQ_REMOVE(&manager->m_to_notify_testers, tester, m_next);
    }
    
    TAILQ_REMOVE(&manager->m_testers, tester, m_next);
    mem_free(manager->m_alloc, tester);
}

net_ndt7_test_type_t net_ndt7_tester_type(net_ndt7_tester_t tester) {
    return tester->m_type;
}

int net_ndt7_tester_set_type(net_ndt7_tester_t tester, net_ndt7_test_type_t test_type) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_state != net_ndt7_tester_state_init) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: can`t set test type in state %s",
            tester->m_id, net_ndt7_tester_state_str(tester->m_state));
        return -1;
    }

    tester->m_type = test_type;
    return 0;
}

net_ndt7_test_protocol_t net_ndt7_tester_protocol(net_ndt7_tester_t tester) {
    return tester->m_protocol;
}

int net_ndt7_tester_set_protocol(net_ndt7_tester_t tester, net_ndt7_test_protocol_t protocol) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_state != net_ndt7_tester_state_init) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: can`t set test protocol in state %s",
            tester->m_id, net_ndt7_tester_state_str(tester->m_state));
        return -1;
    }

    tester->m_protocol = protocol;
    return 0;
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

    if (TAILQ_EMPTY(&tester->m_targets)) {
        net_ndt7_tester_set_state(tester, net_ndt7_tester_state_query_target);
        if (net_ndt7_tester_query_target_start(tester) != 0) {
            if (tester->m_error.m_error == net_ndt7_tester_error_none) {
                net_ndt7_tester_set_error_internal(tester, "start query target fail");
            }
            return -1;
        }
        return 0;
    }

    return net_ndt7_tester_check_start_next_step(tester);
}

net_ndt7_tester_state_t net_ndt7_tester_error_state(net_ndt7_tester_t tester) {
    return tester->m_error.m_state;
}

net_ndt7_tester_error_t net_ndt7_tester_error(net_ndt7_tester_t tester) {
    return tester->m_error.m_error;
}

const char * net_ndt7_tester_error_msg(net_ndt7_tester_t tester) {
    return tester->m_error.m_msg;
}

int net_ndt7_tester_check_start_next_step(net_ndt7_tester_t tester) {
    if (tester->m_error.m_error != net_ndt7_tester_error_none) {
        net_ndt7_tester_set_state(tester, net_ndt7_tester_state_done);
        return 0;
    }
    
    assert(!TAILQ_EMPTY(&tester->m_targets));
    if (tester->m_target == NULL) {
        TAILQ_FOREACH(tester->m_target, &tester->m_targets, m_next) {
            
        }
    }
    
    switch(tester->m_state) {
    case net_ndt7_tester_state_init:
    case net_ndt7_tester_state_query_target:
        switch(tester->m_type) {
        case net_ndt7_test_download:
        case net_ndt7_test_download_and_upload:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_download);
            if (net_ndt7_tester_download_start(tester) != 0) {
                if (tester->m_error.m_error == net_ndt7_tester_error_none) {
                    net_ndt7_tester_set_error_internal(tester, "start download fail");
                }
                net_ndt7_tester_set_state(tester, net_ndt7_tester_state_done);
                return -1;
            }
            return 0;
        case net_ndt7_test_upload:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_upload);
            if (net_ndt7_tester_upload_start(tester) != 0) {
                if (tester->m_error.m_error == net_ndt7_tester_error_none) {
                    net_ndt7_tester_set_error_internal(tester, "start upload fail");
                }
                return -1;
            }
            return 0;
        }
    case net_ndt7_tester_state_download:
        switch(tester->m_type) {
        case net_ndt7_test_upload:
            assert(0);
            return 0;
        case net_ndt7_test_download:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_done);
            return 0;
        case net_ndt7_test_download_and_upload:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_upload);
            if (net_ndt7_tester_upload_start(tester) != 0) {
                if (tester->m_error.m_error == net_ndt7_tester_error_none) {
                    net_ndt7_tester_set_error_internal(tester, "start upload fail");
                }
                net_ndt7_tester_set_state(tester, net_ndt7_tester_state_done);
                return -1;
            }
            return 0;
        }
    case net_ndt7_tester_state_upload:
        switch(tester->m_type) {
        case net_ndt7_test_upload:
        case net_ndt7_test_download_and_upload:
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_done);
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
            net_endpoint_t base_endpoint = net_http_endpoint_base_endpoint(tester->m_state_data.m_query_target.m_endpoint);
            net_endpoint_set_data_watcher(base_endpoint, NULL, NULL, NULL);
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
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

void net_ndt7_tester_set_error(net_ndt7_tester_t tester, net_ndt7_tester_error_t err, const char * msg) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_error.m_msg) {
        mem_free(manager->m_alloc, tester->m_error.m_msg);
        tester->m_error.m_msg = NULL;
    }

    tester->m_error.m_state = tester->m_state;
    tester->m_error.m_error = err;
    tester->m_error.m_msg = msg ? cpe_str_mem_dup(manager->m_alloc, msg) : NULL;
}

void net_ndt7_tester_set_error_internal(net_ndt7_tester_t tester, const char * msg) {
    net_ndt7_tester_set_error(tester, net_ndt7_tester_error_internal, msg);
}

static void net_ndt7_tester_set_state(net_ndt7_tester_t tester, net_ndt7_tester_state_t state) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_state == state) return;

    net_ndt7_tester_clear_state_data(tester);

    CPE_INFO(
        manager->m_em, "ndt: %d: state %s ==> %s",
        tester->m_id, net_ndt7_tester_state_str(tester->m_state), net_ndt7_tester_state_str(state));

    tester->m_state = state;

    if (tester->m_state == net_ndt7_tester_state_done) {
        assert(tester->m_is_to_notify == 0);

        if (TAILQ_EMPTY(&manager->m_to_notify_testers)) {
            net_timer_active(manager->m_delay_process, 0);
        }

        tester->m_is_to_notify = 1;
        TAILQ_INSERT_TAIL(&manager->m_to_notify_testers, tester, m_next_for_notify);
    }
}

void net_ndt7_tester_notify_complete(net_ndt7_tester_t tester) {
    assert(tester->m_state == net_ndt7_tester_state_done);
    
    if (tester->m_on_complete) {
        uint8_t tag_local = 0;
        if (!tester->m_is_processing) {
            tester->m_is_processing = 1;
            tag_local = 1;
        }

        tester->m_on_complete(tester->m_ctx, tester);

        if (tag_local) {
            tester->m_is_processing = 0;
            if (tester->m_is_free) {
                net_ndt7_tester_free(tester);
            }
        }
    }
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

const char * net_ndt7_tester_error_str(net_ndt7_tester_error_t err) {
    switch(err) {
    case net_ndt7_tester_error_none:
        return "none";
    case net_ndt7_tester_error_timeout:
        return "timeout";
    case net_ndt7_tester_error_cancel:
        return "cancel";
    case net_ndt7_tester_error_network:
        return "network";
    case net_ndt7_tester_error_internal:
        return "internal";
    }
}
