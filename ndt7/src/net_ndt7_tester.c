#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/url.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "net_timer.h"
#include "net_endpoint.h"
#include "net_endpoint_monitor.h"
#include "net_ws_endpoint.h"
#include "net_http_req.h"
#include "net_http_endpoint.h"
#include "net_ndt7_tester_i.h"
#include "net_ndt7_tester_target_i.h"

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
    tester->m_protocol = net_ndt7_test_protocol_auto;
    tester->m_state = net_ndt7_tester_state_init;
    tester->m_is_processing = 0;
    tester->m_is_free = 0;

    tester->m_cfg = manager->m_cfg;
    tester->m_is_to_notify = 0;

    tester->m_query_target.m_endpoint = NULL;
    tester->m_query_target.m_req = NULL;

    tester->m_download.m_start_time_ms = 0;
    tester->m_download.m_pre_notify_ms = 0;
    tester->m_download.m_num_bytes = 0.0;
    tester->m_download.m_completed = 0;
    tester->m_download.m_endpoint = NULL;

    tester->m_upload.m_start_time_ms = 0;
    tester->m_upload.m_pre_notify_ms = 0;
    tester->m_upload.m_package_size = 0;
    tester->m_upload.m_total_bytes_queued = 0;
    tester->m_upload.m_completed = 0;
    tester->m_upload.m_endpoint = NULL;
    tester->m_upload.m_process_timer = NULL;
    
    tester->m_error.m_state = net_ndt7_tester_state_init;
    tester->m_error.m_error = net_ndt7_tester_error_none;
    tester->m_error.m_msg = NULL;

    tester->m_ctx = NULL;
    tester->m_on_speed_progress = NULL;
    tester->m_on_measurement_progress = NULL;
    tester->m_on_test_complete = NULL;
    tester->m_on_all_complete = NULL;
    tester->m_ctx_free = NULL;

    tester->m_target = NULL;
    tester->m_effect_protocol = net_ndt7_test_protocol_auto;
    tester->m_upload_url = NULL;
    tester->m_download_url = NULL;
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
    assert(tester->m_target == NULL);
    assert(tester->m_upload_url == NULL);
    assert(tester->m_download_url == NULL);

    if (tester->m_query_target.m_req) {
        net_http_req_clear_reader(tester->m_query_target.m_req);
        tester->m_query_target.m_req = NULL;
    }

    if (tester->m_query_target.m_endpoint) {
        net_endpoint_t base_endpoint = net_http_endpoint_base_endpoint(tester->m_query_target.m_endpoint);
        net_endpoint_set_data_watcher(base_endpoint, NULL, NULL, NULL);
        net_endpoint_monitor_free_by_ctx(base_endpoint, tester);
        net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        tester->m_query_target.m_endpoint = NULL;
    }

    if (tester->m_download.m_endpoint) {
        net_endpoint_t base_endpoint = net_ws_endpoint_base_endpoint(tester->m_download.m_endpoint);
        net_endpoint_set_data_watcher(base_endpoint, NULL, NULL, NULL);
        net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        tester->m_download.m_endpoint = NULL;
    }

    if (tester->m_upload.m_endpoint) {
        net_endpoint_t base_endpoint = net_ws_endpoint_base_endpoint(tester->m_upload.m_endpoint);
        net_endpoint_set_data_watcher(base_endpoint, NULL, NULL, NULL);
        net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        tester->m_upload.m_endpoint = NULL;
    }

    if (tester->m_upload.m_process_timer) {
        net_timer_free(tester->m_upload.m_process_timer);
        tester->m_upload.m_process_timer = NULL;
    }

    if (tester->m_ctx_free) {
        tester->m_ctx_free(tester->m_ctx);
        tester->m_ctx_free = NULL;
        tester->m_ctx = NULL;
    }
    tester->m_on_speed_progress = NULL;
    tester->m_on_measurement_progress = NULL;
    tester->m_on_test_complete = NULL;
    tester->m_on_all_complete = NULL;

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

net_ndt7_config_t net_ndt7_tester_config(net_ndt7_tester_t tester) {
    return &tester->m_cfg;
}

void net_ndt7_tester_set_config(net_ndt7_tester_t tester, net_ndt7_config_t config) {
    tester->m_cfg = *config;
}

void net_ndt7_tester_set_cb(
    net_ndt7_tester_t tester,
    void * ctx,
    net_ndt7_tester_on_speed_progress_fun_t on_speed_progress,
    net_ndt7_tester_on_measurement_progress_fun_t on_measurement_progress,
    net_ndt7_tester_on_test_complete_fun_t on_test_complete,
    net_ndt7_tester_on_all_complete_fun_t on_all_complete,
    void (*ctx_free)(void *))
{
    if (tester->m_ctx_free) {
        tester->m_ctx_free(tester->m_ctx);
    }

    tester->m_ctx = ctx;
    tester->m_ctx_free = ctx_free;
    tester->m_on_speed_progress = on_speed_progress;
    tester->m_on_measurement_progress = on_measurement_progress;
    tester->m_on_test_complete = on_test_complete;
    tester->m_on_all_complete = on_all_complete;
}
    
void net_ndt7_tester_clear_cb(net_ndt7_tester_t tester) {
    tester->m_ctx = NULL;
    tester->m_ctx_free = NULL;
    tester->m_on_speed_progress = NULL;
    tester->m_on_measurement_progress = NULL;
    tester->m_on_test_complete = NULL;
    tester->m_on_all_complete = NULL;
}

const char * net_ndt7_tester_effect_machine(net_ndt7_tester_t tester) {
    return tester->m_target ? tester->m_target->m_machine : NULL;
}

const char * net_ndt7_tester_effect_country(net_ndt7_tester_t tester) {
    return tester->m_target ? tester->m_target->m_country : NULL;
}

const char * net_ndt7_tester_effect_city(net_ndt7_tester_t tester) {
    return tester->m_target ? tester->m_target->m_city : NULL;
}

net_ndt7_test_protocol_t net_ndt7_tester_effect_protocol(net_ndt7_tester_t tester) {
    return tester->m_effect_protocol;
}

int net_ndt7_tester_set_target(net_ndt7_tester_t tester, cpe_url_t url) {
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_state != net_ndt7_tester_state_init) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: set target: can`t set in state %s",
            tester->m_id, net_ndt7_tester_state_str(tester->m_state));
        return -1;
    }

    if (tester->m_target != NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: set target: already have target", tester->m_id);
        return -1;
    }

    net_ndt7_test_protocol_t effect_protocol;
    net_ndt7_target_url_category_t upload_category;
    net_ndt7_target_url_category_t download_category;
    if (cpe_str_cmp_opt(cpe_url_protocol(url), "ws") == 0) {
        effect_protocol = net_ndt7_test_protocol_ws;
        upload_category = net_ndt7_target_url_ws_upload;
        download_category = net_ndt7_target_url_ws_download;
    }
    else if (cpe_str_cmp_opt(cpe_url_protocol(url), "wss") != 0) {
        effect_protocol = net_ndt7_test_protocol_wss;
        upload_category = net_ndt7_target_url_wss_upload;
        download_category = net_ndt7_target_url_wss_download;
    }
    else {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: set target: protocol %s not support",
            tester->m_id, cpe_url_protocol(url));
        return -1;
    }
    
    net_ndt7_tester_target_t target = net_ndt7_tester_target_create(tester);
    if (target == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: set target: create target fail", tester->m_id);
        return -1;
    }

    if (cpe_url_host(url)) {
        net_ndt7_tester_target_set_machine(target, cpe_url_host(url));
    }

    mem_buffer_t buffer = net_ndt7_manage_tmp_buffer(manager);
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    
    mem_buffer_clear_data(buffer);
    cpe_url_print((write_stream_t)&stream, url, cpe_url_print_full);
    stream_printf((write_stream_t)&stream, "/ndt/v7/upload");
    stream_putc((write_stream_t)&stream, 0);

    const char * upload_url = mem_buffer_make_continuous(buffer, 0);
    if (net_ndt7_tester_target_set_url(target, upload_category, upload_url) != 0) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: set target: set upload url %s target fail",
            tester->m_id, upload_url);
        net_ndt7_tester_target_free(target);
        return -1;
    }

    mem_buffer_clear_data(buffer);
    cpe_url_print((write_stream_t)&stream, url, cpe_url_print_full);
    stream_printf((write_stream_t)&stream, "/ndt/v7/download");
    stream_putc((write_stream_t)&stream, 0);

    const char * download_url = mem_buffer_make_continuous(buffer, 0);
    if (net_ndt7_tester_target_set_url(target, download_category, download_url) != 0) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: set target: set download url %s target fail",
            tester->m_id, download_url);
        net_ndt7_tester_target_free(target);
        return -1;
    }
    
    tester->m_target = target;
    tester->m_effect_protocol = effect_protocol;
    tester->m_upload_url = net_ndt7_tester_target_url(target, upload_category);
    assert(tester->m_upload_url);
    
    tester->m_download_url = net_ndt7_tester_target_url(target, download_category);
    assert(tester->m_download_url);
    
    return 0;
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
    net_ndt7_manage_t manager = tester->m_manager;

    if (tester->m_error.m_error != net_ndt7_tester_error_none) {
        net_ndt7_tester_set_state(tester, net_ndt7_tester_state_done);
        return 0;
    }
    
    assert(!TAILQ_EMPTY(&tester->m_targets));
    if (tester->m_target == NULL) {
        TAILQ_FOREACH(tester->m_target, &tester->m_targets, m_next) {
            uint8_t found = 0;

            switch(tester->m_type) {
            case net_ndt7_test_download:
                tester->m_download_url = net_ndt7_tester_target_select_download_url(tester->m_target, tester->m_protocol);
                if (tester->m_download_url) found = 1;
                break;
            case net_ndt7_test_download_and_upload:
                tester->m_download_url = net_ndt7_tester_target_select_download_url(tester->m_target, tester->m_protocol);
                if (tester->m_download_url) {
                    tester->m_upload_url = net_ndt7_tester_target_select_upload_url(tester->m_target, tester->m_protocol);
                    if (tester->m_upload_url) found = 1;
                }
                break;
            case net_ndt7_test_upload:
                tester->m_upload_url = net_ndt7_tester_target_select_upload_url(tester->m_target, tester->m_protocol);
                if (tester->m_upload_url) found = 1;
                break;
            }

            if (found) {
                if (tester->m_upload_url) {
                    tester->m_effect_protocol = 
                        tester->m_upload_url == tester->m_target->m_urls[net_ndt7_target_url_wss_upload]
                        ? net_ndt7_test_protocol_wss : net_ndt7_test_protocol_ws;
                }
                else if (tester->m_download_url) {
                    tester->m_effect_protocol = 
                        tester->m_download_url == tester->m_target->m_urls[net_ndt7_target_url_wss_download]
                        ? net_ndt7_test_protocol_wss : net_ndt7_test_protocol_ws;
                }
                
                break;
            }
        }

        if (tester->m_target == NULL) {
            CPE_ERROR(
                manager->m_em, "ndt7: %d: select target fail, target-count=%d",
                tester->m_id, net_ndt7_tester_target_count(tester));
            
            if (tester->m_error.m_error == net_ndt7_tester_error_none) {
                net_ndt7_tester_set_error_internal(tester, "SelectTargetFail");
            }
            net_ndt7_tester_set_state(tester, net_ndt7_tester_state_done);
            return -1;
        }
    }

    assert(tester->m_effect_protocol != net_ndt7_test_protocol_auto);
    
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
    
    if (tester->m_on_all_complete) {
        uint8_t tag_local = 0;
        if (!tester->m_is_processing) {
            tester->m_is_processing = 1;
            tag_local = 1;
        }

        tester->m_on_all_complete(tester->m_ctx, tester);

        if (tag_local) {
            tester->m_is_processing = 0;
            if (tester->m_is_free) {
                net_ndt7_tester_free(tester);
            }
        }
    }
}

void net_ndt7_tester_notify_speed_progress(net_ndt7_tester_t tester, net_ndt7_response_t response) {
    if (tester->m_on_speed_progress) {
        tester->m_on_speed_progress(tester->m_ctx, tester, response);
    }
}

void net_ndt7_tester_notify_measurement_progress(net_ndt7_tester_t tester, net_ndt7_measurement_t measurement) {
    if (tester->m_on_measurement_progress) {
        tester->m_on_measurement_progress(tester->m_ctx, tester, measurement);
    }
}

void net_ndt7_tester_notify_test_complete(
    net_ndt7_tester_t tester,
    net_endpoint_error_source_t error_source, int error_code, const char * error_msg,
    net_ndt7_response_t response, net_ndt7_test_type_t test_type)
{
    if (tester->m_on_test_complete) {
        tester->m_on_test_complete(tester->m_ctx, tester, error_source, error_code, error_msg, response, test_type);
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

const char * net_ndt7_test_protocol_str(net_ndt7_test_protocol_t protocol) {
    switch(protocol) {
    case net_ndt7_test_protocol_auto:
        return "auto";
    case net_ndt7_test_protocol_ws:
        return "ws";
    case net_ndt7_test_protocol_wss:
        return "wss";
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
