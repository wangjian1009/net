#include "cpe/utils/url.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream.h"
#include "net_ndt7_tester_target_i.h"

net_ndt7_tester_target_t
net_ndt7_tester_target_create(net_ndt7_tester_t tester) {
    net_ndt7_manage_t manager = tester->m_manager;

    net_ndt7_tester_target_t target = mem_alloc(manager->m_alloc, sizeof(struct net_ndt7_tester_target));
    if (target == NULL) {
        CPE_ERROR(manager->m_em, "ndt7: %d: target: alloc fail", tester->m_id);
        return NULL;
    }

    target->m_tester = tester;
    target->m_machine = NULL;
    target->m_country = NULL;
    target->m_city = NULL;
    
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

    if (tester->m_target == target) {
        tester->m_target = NULL;
        tester->m_download_url = NULL;
        tester->m_upload_url = NULL;
    }
    
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

uint16_t net_ndt7_tester_target_count(net_ndt7_tester_t tester) {
    uint16_t count = 0;

    net_ndt7_tester_target_t target;
    TAILQ_FOREACH(target, &tester->m_targets, m_next) {
        count++;
    }

    return count;
}

net_ndt7_tester_target_t net_ndt7_tester_next_target(net_ndt7_tester_target_it_t it) {
    net_ndt7_tester_target_t * pr = (void*)it->data;

    net_ndt7_tester_target_t r = *pr;
    if (r) {
        *pr = TAILQ_NEXT(r, m_next);
    }

    return r;
}
    
void net_ndt7_tester_targets(net_ndt7_tester_t tester, net_ndt7_tester_target_it_t it) {
    *((net_ndt7_tester_target_t *)it->data) = TAILQ_FIRST(&tester->m_targets);
    it->next = net_ndt7_tester_next_target;
}

const char * net_ndt7_tester_target_machine(net_ndt7_tester_target_t target) {
    return target->m_machine;
}

int net_ndt7_tester_target_set_machine(net_ndt7_tester_target_t target, const char * machine) {
    net_ndt7_tester_t tester = target->m_tester;
    net_ndt7_manage_t manager = target->m_tester->m_manager;
    
    char * new_value = NULL;
    if (machine) {
        new_value = cpe_str_mem_dup(manager->m_alloc, machine);
        if (new_value == NULL) {
            CPE_ERROR(manager->m_em, "ndt7: %d: target: dup machine %s fail", tester->m_id, machine);
            return -1;
        }
    }

    if (target->m_machine) {
        mem_free(manager->m_alloc, target->m_machine);
    }

    target->m_machine = new_value;

    return 0;
}

const char * net_ndt7_tester_target_country(net_ndt7_tester_target_t target) {
    return target->m_country;
}

int net_ndt7_tester_target_set_country(net_ndt7_tester_target_t target, const char * country) {
    net_ndt7_tester_t tester = target->m_tester;
    net_ndt7_manage_t manager = target->m_tester->m_manager;
    
    char * new_value = NULL;
    if (country) {
        new_value = cpe_str_mem_dup(manager->m_alloc, country);
        if (new_value == NULL) {
            CPE_ERROR(manager->m_em, "ndt7: %d: target: dup country %s fail", tester->m_id, country);
            return -1;
        }
    }

    if (target->m_country) {
        mem_free(manager->m_alloc, target->m_country);
    }

    target->m_country = new_value;

    return 0;
}

const char * net_ndt7_tester_target_city(net_ndt7_tester_target_t target) {
    return target->m_city;
}

int net_ndt7_tester_target_set_city(net_ndt7_tester_target_t target, const char * city) {
    net_ndt7_tester_t tester = target->m_tester;
    net_ndt7_manage_t manager = target->m_tester->m_manager;
    
    char * new_value = NULL;
    if (city) {
        new_value = cpe_str_mem_dup(manager->m_alloc, city);
        if (new_value == NULL) {
            CPE_ERROR(manager->m_em, "ndt7: %d: target: dup city %s fail", tester->m_id, city);
            return -1;
        }
    }

    if (target->m_city) {
        mem_free(manager->m_alloc, target->m_city);
    }

    target->m_city = new_value;

    return 0;
}

cpe_url_t net_ndt7_tester_target_url(net_ndt7_tester_target_t target, net_ndt7_target_url_category_t category) {
    return target->m_urls[category];
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

cpe_url_t net_ndt7_tester_target_select_upload_url(
    net_ndt7_tester_target_t target, net_ndt7_test_protocol_t protocol)
{
    switch(protocol) {
    case net_ndt7_test_protocol_auto:
        if (target->m_urls[net_ndt7_target_url_wss_upload]) {
            return target->m_urls[net_ndt7_target_url_wss_upload];
        }

        if (target->m_urls[net_ndt7_target_url_ws_upload]) {
            return target->m_urls[net_ndt7_target_url_ws_upload];
        }
        break;
    case net_ndt7_test_protocol_ws:
        if (target->m_urls[net_ndt7_target_url_ws_upload]) {
            return target->m_urls[net_ndt7_target_url_ws_upload];
        }
        break;
    case net_ndt7_test_protocol_wss:
        if (target->m_urls[net_ndt7_target_url_wss_upload]) {
            return target->m_urls[net_ndt7_target_url_wss_upload];
        }
        break;
    }

    return NULL;
}

cpe_url_t net_ndt7_tester_target_select_download_url(
    net_ndt7_tester_target_t target, net_ndt7_test_protocol_t protocol)
{
    switch(protocol) {
    case net_ndt7_test_protocol_auto:
        if (target->m_urls[net_ndt7_target_url_wss_download]) {
            return target->m_urls[net_ndt7_target_url_wss_download];
        }

        if (target->m_urls[net_ndt7_target_url_ws_download]) {
            return target->m_urls[net_ndt7_target_url_ws_download];
        }
        break;
    case net_ndt7_test_protocol_ws:
        if (target->m_urls[net_ndt7_target_url_ws_download]) {
            return target->m_urls[net_ndt7_target_url_ws_download];
        }
        break;
    case net_ndt7_test_protocol_wss:
        if (target->m_urls[net_ndt7_target_url_wss_download]) {
            return target->m_urls[net_ndt7_target_url_wss_download];
        }
        break;
    }

    return NULL;
}

const char * net_ndt7_target_url_category_str(net_ndt7_target_url_category_t category) {
    switch(category) {
    case net_ndt7_target_url_ws_upload:
        return "ws-upload";
    case net_ndt7_target_url_ws_download:
        return "ws-download";
    case net_ndt7_target_url_wss_upload:
        return "wss-upload";
    case net_ndt7_target_url_wss_download:
        return "wss-download";
    }
}
