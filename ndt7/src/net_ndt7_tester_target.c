#include "cpe/utils/url.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream.h"
#include "net_ndt7_tester_target_i.h"

net_ndt7_tester_target_t
net_ndt7_tester_target_create(
    net_ndt7_tester_t tester,
    const char * machine, const char * country, const char * city)
{
    net_ndt7_manage_t manager = tester->m_manager;

    net_ndt7_tester_target_t target = mem_alloc(manager->m_alloc, sizeof(struct net_ndt7_tester_target));
    if (target == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: target: alloc fail", tester->m_id);
        return NULL;
    }

    target->m_tester = tester;

    target->m_machine = cpe_str_mem_dup(manager->m_alloc, machine);
    if (target->m_machine == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: target: dup machine %s fail", tester->m_id, machine);
        mem_free(manager->m_alloc, target);
        return NULL;
    }

    target->m_country = NULL;
    if (country) {
        target->m_country = cpe_str_mem_dup(manager->m_alloc, country);
        if (target->m_country == NULL) {
            CPE_ERROR(manager->m_em, "ndt7: %d: target: dup country %s fail", tester->m_id, country);
            mem_free(manager->m_alloc, target->m_machine);
            mem_free(manager->m_alloc, target);
            return NULL;
        }
    }

    target->m_city = NULL;
    if (city) {
        target->m_city = cpe_str_mem_dup(manager->m_alloc, city);
        if (target->m_city == NULL) {
            CPE_ERROR(manager->m_em, "ndt7: %d: target: dup city %s fail", tester->m_id, city);
            if (target->m_country) mem_free(manager->m_alloc, target->m_country);
            mem_free(manager->m_alloc, target->m_machine);
            mem_free(manager->m_alloc, target);
            return NULL;
        }
    }
    
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(target->m_urls); i++) {
        target->m_urls[i] = NULL;
    }

    TAILQ_INSERT_TAIL(&tester->m_targets, target, m_next);
    
    return target;
}

void net_ndt7_tester_target_free(net_ndt7_tester_target_t target) {
    net_ndt7_tester_t tester = target->m_tester;
    net_ndt7_manage_t manager = tester->m_manager;

    if (target->m_machine) {
        mem_free(manager->m_alloc, target->m_machine);
        target->m_machine = NULL;
    }

    if (target->m_city) {
        mem_free(manager->m_alloc, target->m_city);
        target->m_city = NULL;
    }

    if (target->m_country) {
        mem_free(manager->m_alloc, target->m_country);
        target->m_country = NULL;
    }
    
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(target->m_urls); i++) {
        if (target->m_urls[i]) {
            cpe_url_free(target->m_urls[i]);
            target->m_urls[i] = NULL;
        }
    }
    
    TAILQ_REMOVE(&tester->m_targets, target, m_next);

    mem_free(manager->m_alloc, target);
}

int net_ndt7_tester_target_set_url(
    net_ndt7_tester_target_t target, net_ndt7_target_url_category_t category, const char * str_url)
{
    net_ndt7_tester_t tester = target->m_tester;
    net_ndt7_manage_t manager = target->m_tester->m_manager;

    cpe_url_t url = cpe_url_parse(manager->m_alloc, manager->m_em, str_url);
    if (url == NULL) {
        CPE_ERROR(
            manager->m_em, "ndt7: %d: target %s: parse url %s fail",
            tester->m_id, target->m_machine, str_url);
        return -1;
    }

    if (target->m_urls[category]) {
        cpe_url_free(target->m_urls[category]);
    }

    target->m_urls[category] = url;
    return 0;
}


void net_ndt7_tester_target_print(write_stream_t ws, net_ndt7_tester_target_t target) {
    stream_printf(ws, "%s@%s.%s", target->m_machine, target->m_country, target->m_city);

    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(target->m_urls); i++) {
        stream_printf(ws, "\n");
        stream_putc_count(ws, ' ', 4);
        stream_printf(ws, "%s: ", net_ndt7_target_url_category_str(i));
        if (target->m_urls[i]) {
            cpe_url_print(ws, target->m_urls[i], cpe_url_print_full);
        }
        else {
            stream_printf(ws, "N/A");
        }
    }
}

const char * net_ndt7_target_url_category_str(net_ndt7_target_url_category_t category) {
    switch(category) {
    case net_ndt7_target_url_ws_uploded:
        return "ws-upload";
    case net_ndt7_target_url_ws_download:
        return "ws-download";
    case net_ndt7_target_url_wss_uploded:
        return "wss-upload";
    case net_ndt7_target_url_wss_download:
        return "wss-download";
    }
}
