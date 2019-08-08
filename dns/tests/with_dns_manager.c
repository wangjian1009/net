#include "yaml_parse.h"
#include "yaml_utils.h"
#include "net_schedule.h"
#include "net_address.h"
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
    net_dns_manage_set_debug(with_dns_manager->m_dns, 1);
}

void with_dns_manager_teardown(void) {
    ck_assert(with_dns_manager != NULL);

    net_dns_manage_free(with_dns_manager->m_dns);
    net_schedule_free(with_dns_manager->m_schedule);
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
        switch(child->type) {
        case YAML_SCALAR_NODE: {
            net_address_t resolved = net_address_create_auto(with_dns_manager->m_schedule, yaml_node_value(child));
            ck_assert(resolved);

            ck_assert(
                net_dns_manage_add_record(with_dns_manager->m_dns, host, resolved, 10000) == 0);
            
            net_address_free(resolved);
            break;
        }
        case YAML_SEQUENCE_NODE:
            break;
        default:
            break;
        }
        
        net_address_free(host);
    }

    yaml_document_delete(&document);
}
