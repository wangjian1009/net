#include "yaml_parse.h"
#include "yaml_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_dns_entry.h"
#include "with_dns_manager.h"

with_dns_manager_t with_dns_manager = NULL;

void with_dns_manager_setup(void) {
    ck_assert(with_dns_manager == NULL);
    
    with_dns_manager = mem_alloc(NULL, sizeof(struct with_dns_manager));
    ck_assert(with_dns_manager != NULL);
    
    with_dns_manager->m_alloc = NULL;
    cpe_error_monitor_init(&with_dns_manager->m_em_buf, cpe_error_log_to_consol, NULL);

    with_dns_manager->m_schedule = net_schedule_create(
        with_dns_manager->m_alloc, &with_dns_manager->m_em_buf, 5 * 1024 * 1025);
    ck_assert(with_dns_manager->m_schedule);
    
    with_dns_manager->m_dns = 
        net_dns_manage_create(with_dns_manager->m_alloc, &with_dns_manager->m_em_buf, with_dns_manager->m_schedule);
    ck_assert(with_dns_manager->m_dns);
    //net_dns_manage_set_debug(with_dns_manager->m_dns, 1);
    
    mem_buffer_init(&with_dns_manager->m_tmp_buffer, with_dns_manager->m_alloc);
}

void with_dns_manager_teardown(void) {
    ck_assert(with_dns_manager != NULL);

    net_dns_manage_free(with_dns_manager->m_dns);
    net_schedule_free(with_dns_manager->m_schedule);
    mem_buffer_clear(&with_dns_manager->m_tmp_buffer);
    mem_free(with_dns_manager->m_alloc, with_dns_manager);
    with_dns_manager = NULL;
}

void with_dns_manager_add_records(const char * str_records) {
    ck_assert(with_dns_manager);
    
    char error_buf[128];

    yaml_document_t document;
    ck_assert_msg(
        yajl_document_parse_one(&document, str_records, error_buf, sizeof(error_buf)) == 0,
        "parse yaml fail\ninput:\n%s\nerror: %s", str_records, error_buf);

    yaml_node_t * root = yaml_document_get_root_node(&document);
    ck_assert(root);

    struct yaml_node_mapping_item_it node_it;
    yaml_node_mapping_childs(&document, root, &node_it);

    yaml_node_mapping_item_t child_node;
    while((child_node = yaml_node_mapping_item_it_next(&node_it)) != NULL) {
        net_address_t host = net_address_create_auto(with_dns_manager->m_schedule, yaml_node_mapping_item_name(child_node));
        ck_assert(host);
        
        yaml_node_t * child = yaml_node_mapping_item_value(child_node);
        if (child->type == YAML_SCALAR_NODE) {
            net_address_t resolved = net_address_create_auto(with_dns_manager->m_schedule, yaml_node_value(child));
            ck_assert(resolved);

            ck_assert(
                net_dns_manage_add_record(with_dns_manager->m_dns, host, resolved, 10000) == 0);
            
            net_address_free(resolved);
        }
        else if (child->type == YAML_SEQUENCE_NODE) {
            struct yaml_node_it resolved_it;
            yaml_node_sequence_childs(&document, child, &resolved_it);

            yaml_node_t * resolved_node;
            while((resolved_node = yaml_node_it_next(&resolved_it))) {
                net_address_t resolved = net_address_create_auto(with_dns_manager->m_schedule, yaml_node_value(resolved_node));
                ck_assert(resolved);

                ck_assert(
                    net_dns_manage_add_record(with_dns_manager->m_dns, host, resolved, 10000) == 0);

                net_address_free(resolved);
            }
        }
        
        net_address_free(host);
    }

    yaml_document_delete(&document);
}

net_dns_entry_t with_dns_manager_find_entry(const char * str_address) {
    ck_assert(with_dns_manager);

    net_address_t address = net_address_create_auto(with_dns_manager->m_schedule, str_address);
    ck_assert_msg(address, "create address from %s fail", str_address);
    
    net_dns_entry_t r = net_dns_entry_find(with_dns_manager->m_dns, address);
    net_address_free(address);
    return r;
}

const char * with_dns_manager_hostnames_by_ip(const char * str_ip) {
    ck_assert(with_dns_manager);
    
    net_address_t address = net_address_create_auto(with_dns_manager->m_schedule, str_ip);
    ck_assert_msg(address, "create address from %s fail", str_ip);

    struct net_address_it address_it;
    net_dns_hostnames_by_ip(&address_it, with_dns_manager->m_dns, address);
    net_address_free(address);

    mem_buffer_clear(&with_dns_manager->m_tmp_buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(&with_dns_manager->m_tmp_buffer);
    
    uint16_t i = 0;
    while((address = net_address_it_next(&address_it))) {
        if(i++ > 0) stream_printf((write_stream_t)&ws, ",");
        net_address_print((write_stream_t)&ws, address);
    }
    
    mem_buffer_append_char(&with_dns_manager->m_tmp_buffer, 0);
    return mem_buffer_make_continuous(&with_dns_manager->m_tmp_buffer, 0);
}

const char * with_dns_manager_hostname_by_ip(const char * str_ip) {
    ck_assert(with_dns_manager);
    
    net_address_t address = net_address_create_auto(with_dns_manager->m_schedule, str_ip);
    ck_assert_msg(address, "create address from %s fail", str_ip);

    net_address_t r = net_dns_hostname_by_ip(with_dns_manager->m_dns, address);
    net_address_free(address);
    
    return r ? net_address_dump(&with_dns_manager->m_tmp_buffer, r) : NULL;
}
