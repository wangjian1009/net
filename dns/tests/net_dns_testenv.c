#include "cmocka_all.h"
#include "yaml_parse.h"
#include "yaml_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_dns_entry.h"
#include "net_dns_entry_item.h"
#include "net_dns_testenv.h"

net_dns_testenv_t net_dns_testenv_create() {
    net_dns_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_dns_testenv));
    assert_true(env != NULL);
    
    env->m_alloc = NULL;
    cpe_error_monitor_init(&env->m_em_buf, cpe_error_log_to_consol, NULL);

    env->m_schedule = net_schedule_create(env->m_alloc, &env->m_em_buf, NULL);
    assert_true(env->m_schedule);
    
    env->m_dns = 
        net_dns_manage_create(env->m_alloc, &env->m_em_buf, env->m_schedule);
    assert_true(env->m_dns);
    //net_dns_manage_set_debug(env->m_dns, 1);
    
    mem_buffer_init(&env->m_tmp_buffer, env->m_alloc);

    return env;
}

void net_dns_testenv_free(net_dns_testenv_t env) {
    assert_true(env != NULL);

    net_dns_manage_free(env->m_dns);
    net_schedule_free(env->m_schedule);
    mem_buffer_clear(&env->m_tmp_buffer);
    mem_free(env->m_alloc, env);
    env = NULL;
}

void net_dns_testenv_setup_records(net_dns_testenv_t env, const char * str_records) {
    assert_true(env);
    
    char error_buf[128];

    yaml_document_t document;
    if (yajl_document_parse_one(&document, str_records, error_buf, sizeof(error_buf)) != 0) {
        fail_msg("parse yaml fail\ninput:\n%s\nerror: %s", str_records, error_buf);
    }

    yaml_node_t * root = yaml_document_get_root_node(&document);
    assert_true(root);

    struct yaml_node_mapping_item_it node_it;
    yaml_node_mapping_childs(&document, root, &node_it);

    yaml_node_mapping_item_t child_node;
    while((child_node = yaml_node_mapping_item_it_next(&node_it)) != NULL) {
        net_address_t host = net_address_create_auto(env->m_schedule, yaml_node_mapping_item_name(child_node));
        assert_true(host);
        
        yaml_node_t * child = yaml_node_mapping_item_value(child_node);
        if (child->type == YAML_SCALAR_NODE) {
            net_address_t resolved = net_address_create_auto(env->m_schedule, yaml_node_value(child));
            assert_true(resolved);

            assert_true(
                net_dns_manage_add_record(env->m_dns, host, resolved, 10000) == 0);
            
            net_address_free(resolved);
        }
        else if (child->type == YAML_SEQUENCE_NODE) {
            struct yaml_node_it resolved_it;
            yaml_node_sequence_childs(&document, child, &resolved_it);

            yaml_node_t * resolved_node;
            while((resolved_node = yaml_node_it_next(&resolved_it))) {
                net_address_t resolved = net_address_create_auto(env->m_schedule, yaml_node_value(resolved_node));
                assert_true(resolved);

                assert_true(
                    net_dns_manage_add_record(env->m_dns, host, resolved, 10000) == 0);

                net_address_free(resolved);
            }
        }
        
        net_address_free(host);
    }

    yaml_document_delete(&document);
}

net_dns_entry_t net_dns_testenv_find_entry(net_dns_testenv_t env, const char * str_address) {
    assert_true(env);

    net_address_t address = net_address_create_auto(env->m_schedule, str_address);
    assert_true(address != NULL);
    
    net_dns_entry_t r = net_dns_entry_find(env->m_dns, address);
    net_address_free(address);
    return r;
}

const char * net_dns_testenv_hostnames_by_ip(net_dns_testenv_t env, const char * str_ip) {
    assert_true(env);
    
    net_address_t address = net_address_create_auto(env->m_schedule, str_ip);
    assert_true(address != NULL);

    struct net_address_it address_it;
    net_dns_hostnames_by_ip(&address_it, env->m_dns, address);
    net_address_free(address);

    mem_buffer_clear(&env->m_tmp_buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(&env->m_tmp_buffer);
    
    uint16_t i = 0;
    while((address = net_address_it_next(&address_it))) {
        if(i++ > 0) stream_printf((write_stream_t)&ws, ",");
        net_address_print((write_stream_t)&ws, address);
    }
    
    mem_buffer_append_char(&env->m_tmp_buffer, 0);
    return mem_buffer_make_continuous(&env->m_tmp_buffer, 0);
}

const char * net_dns_testenv_hostname_by_ip(net_dns_testenv_t env, const char * str_ip) {
    assert_true(env);
    
    net_address_t address = net_address_create_auto(env->m_schedule, str_ip);
    assert_true(address);

    net_address_t r = net_dns_hostname_by_ip(env->m_dns, address);
    net_address_free(address);
    
    return r ? net_address_dump(&env->m_tmp_buffer, r) : NULL;
}

const char * net_dns_testenv_entry_select_item(net_dns_testenv_t env, const char * str_entry, net_dns_query_type_t query_type) {
    assert_true(env);
    
    net_dns_entry_t entry = net_dns_testenv_find_entry(env, str_entry);
    assert_true(entry != NULL);
    
    net_dns_entry_item_t item = net_dns_entry_select_item(entry, net_dns_item_select_policy_first, query_type);
    return item ? net_address_dump(&env->m_tmp_buffer, net_dns_entry_item_address(item)) : NULL;
}

int net_dns_testenv_add_record(net_dns_testenv_t env, const char * str_host, const char * str_resolved) {
    net_address_t host = net_address_create_auto(env->m_schedule, str_host);
    assert_true(host != NULL);

    net_address_t resolved = net_address_create_auto(env->m_schedule, str_resolved);
    assert_true(resolved != NULL);

    int rv = net_dns_manage_add_record(env->m_dns, host, resolved, 10000);

    net_address_free(host);
    net_address_free(resolved);

    return rv;
}
