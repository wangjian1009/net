#include <assert.h>
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_http_svr_mount_point_i.h"
#include "net_http_svr_processor_i.h"

net_http_svr_mount_point_t
net_http_svr_mount_point_create(
    net_http_svr_protocol_t service, const char * name, const char * name_end, net_http_svr_mount_point_t parent, 
    void * processor_env, net_http_svr_processor_t processor)
{
    net_http_svr_mount_point_t mount_point;
    size_t name_len = name_end - name;

    if (name_len + 1 > CPE_TYPE_ARRAY_SIZE(struct net_http_svr_mount_point, m_name)) {
        CPE_ERROR(service->m_em, "net_http_svr_mount_point_create: name %s overflow!", name);
        return NULL;
    }
        
    mount_point = TAILQ_FIRST(&service->m_free_mount_points);
    if (mount_point) {
        TAILQ_REMOVE(&service->m_free_mount_points, mount_point, m_next_for_processor);
    }
    else {
        mount_point = mem_alloc(service->m_alloc, sizeof(struct net_http_svr_mount_point));
        if (mount_point == NULL) {
            CPE_ERROR(service->m_em, "net_http_svr_mount_point_create: alloc fail");
            return NULL;
        }
    }

    mount_point->m_service = service;
    memcpy(mount_point->m_name, name, name_len);
    mount_point->m_name[name_len] = 0;
    mount_point->m_name_len = name_len;
    TAILQ_INIT(&mount_point->m_childs);

    mount_point->m_bridger_to = NULL;
    TAILQ_INIT(&mount_point->m_bridger_froms);
    
    mount_point->m_parent = parent;
    if (mount_point->m_parent) {
        TAILQ_INSERT_TAIL(&mount_point->m_parent->m_childs, mount_point, m_next_for_parent);
    }

    mount_point->m_processor_env = processor_env;
    mount_point->m_processor = processor;
    if(mount_point->m_processor) {
        TAILQ_INSERT_TAIL(&processor->m_mount_points, mount_point, m_next_for_processor);
    }

    return mount_point;
}

void net_http_svr_mount_point_free(net_http_svr_mount_point_t mount_point) {
    net_http_svr_protocol_t service = mount_point->m_service;

    net_http_svr_mount_point_set_bridger_to(mount_point, NULL);
    
    while(!TAILQ_EMPTY(&mount_point->m_bridger_froms)) {
        net_http_svr_mount_point_set_bridger_to(TAILQ_FIRST(&mount_point->m_bridger_froms), NULL);
    }

    while(!TAILQ_EMPTY(&mount_point->m_childs)) {
        net_http_svr_mount_point_free(TAILQ_FIRST(&mount_point->m_childs));
    }
    
    if (mount_point->m_service->m_root == mount_point) {
        mount_point->m_service->m_root = NULL;
    }

    if (mount_point->m_parent) {
        TAILQ_REMOVE(&mount_point->m_parent->m_childs, mount_point, m_next_for_parent);
    }

    if (mount_point->m_processor) {
        if (mount_point->m_processor_env) {
            mount_point->m_processor->m_env_clear(mount_point->m_processor->m_ctx, mount_point->m_processor_env);
            mount_point->m_processor_env = NULL;
        }

        TAILQ_REMOVE(&mount_point->m_processor->m_mount_points, mount_point, m_next_for_processor);
    }

    TAILQ_INSERT_TAIL(&service->m_free_mount_points, mount_point, m_next_for_processor);
}

void net_http_svr_mount_point_real_free(net_http_svr_mount_point_t mount_point) {
    TAILQ_REMOVE(&mount_point->m_service->m_free_mount_points, mount_point, m_next_for_processor);
    mem_free(mount_point->m_service->m_alloc, mount_point);
}

int net_http_svr_mount_point_set_bridger_to(net_http_svr_mount_point_t mp, net_http_svr_mount_point_t to) {
    if (to != mp->m_bridger_to) {
        if (mp->m_bridger_to) {
            TAILQ_REMOVE(&mp->m_bridger_to->m_bridger_froms, mp, m_next_bridger_to);
        }

        mp->m_bridger_to = to;
        
        if (mp->m_bridger_to) {
            TAILQ_INSERT_TAIL(&mp->m_bridger_to->m_bridger_froms, mp, m_next_bridger_to);
        }
    }
    
    return 0;
}

int net_http_svr_mount_point_set_bridger_to_by_path(net_http_svr_mount_point_t mp, const char * path) {
    net_http_svr_mount_point_t to = NULL;

    if (path) {
        to = net_http_svr_mount_point_find_by_path(mp->m_service, &path);
        if (to == NULL) {
            CPE_ERROR(mp->m_service->m_em, "http_svr: set_bridger_to: find target mount point %s fail!", path);
            return -1;
        }

        if (path[0]) {
            to = net_http_svr_mount_point_mount(to, path, NULL, NULL);
            if (to == NULL) {
                CPE_ERROR(mp->m_service->m_em, "http_svr: set_bridger_to: create target mount point %s fail!", path);
                return -1;
            }
        }
    }

    return net_http_svr_mount_point_set_bridger_to(mp, to);
}

net_http_svr_mount_point_t net_http_svr_mount_point_find_by_path(net_http_svr_protocol_t service, const char * * path) {
    return net_http_svr_mount_point_find_child_by_path(service->m_root, path);
}

net_http_svr_mount_point_t net_http_svr_mount_point_find_child_by_name(net_http_svr_mount_point_t p, const char * name) {
    net_http_svr_mount_point_t sub_point;
    
    TAILQ_FOREACH(sub_point, &p->m_childs, m_next_for_parent) {
        if (strcmp(sub_point->m_name, name) == 0) return sub_point;
    }

    return NULL;
}

static int net_http_svr_mount_point_find_child_process_mount(net_http_svr_mount_point_t * i_mp, const char ** path, uint8_t * path_in_tmp) {
    net_http_svr_mount_point_t mp = *i_mp;
    while(mp->m_bridger_to) {
        size_t append_path_len;
        net_http_svr_mount_point_t bridger_to = mp->m_bridger_to;
        net_http_svr_mount_point_t next_mp;

        /*当前点走到最后一个转接 */
        while(bridger_to->m_bridger_to) bridger_to = bridger_to->m_bridger_to;

        /*向上搜索， 寻找到拥有processor的挂节点或者新的挂节点 */
        next_mp = bridger_to;
        append_path_len = 0;
        while(next_mp && next_mp->m_processor == NULL && next_mp->m_bridger_to == NULL) {
            append_path_len += next_mp->m_name_len + 1;
            next_mp = next_mp->m_parent;
        }

        /*没有找到则错误返回 */
        if (next_mp == NULL) {
            CPE_ERROR(mp->m_service->m_em, "http_svr: mount_point: find_child_process_mount: can`t found bridge to point with processor!");
            return -1;
        }

        /* 将新增的路径拼接到path中去 */
        if (append_path_len > 0) {
            char * insert_buf;
            net_http_svr_mount_point_t tmp_mp;
            
            if (*path_in_tmp) {
                struct mem_buffer_pos pt;
                mem_buffer_begin(&pt, &mp->m_service->m_search_path_buffer);
                insert_buf = mem_pos_insert_alloc(&pt, append_path_len);
                if (insert_buf == NULL) {
                    CPE_ERROR(mp->m_service->m_em, "http_svr: mount_point: find_child_process_mount: append path, insert alloc fail!");
                    return -1;
                }
            }
            else {
                size_t path_len = strlen(*path) + 1;

                mem_buffer_clear_data(&mp->m_service->m_search_path_buffer);
                
                insert_buf = mem_buffer_alloc(&mp->m_service->m_search_path_buffer, append_path_len + path_len);
                if (insert_buf == NULL) {
                    CPE_ERROR(mp->m_service->m_em, "http_svr: mount_point: find_child_process_mount: append path, init alloc fail!");
                    return -1;
                }

                memcpy(insert_buf + append_path_len, *path, path_len);
                *path_in_tmp = 1;
            }

            for(tmp_mp = bridger_to; tmp_mp != next_mp; tmp_mp = tmp_mp->m_parent) {
                assert(append_path_len >= (tmp_mp->m_name_len + 1));

                append_path_len -= (tmp_mp->m_name_len + 1);
                memcpy(insert_buf + append_path_len, tmp_mp->m_name, tmp_mp->m_name_len);
                insert_buf[append_path_len + tmp_mp->m_name_len] = '/';
            }

            *path = mem_buffer_make_continuous(&mp->m_service->m_search_path_buffer, 0);
        }
        
        if (next_mp->m_processor) {
            *i_mp = next_mp;
            break;
        }

        mp = next_mp;
        assert(mp->m_bridger_to);
    }

    return 0;
}
    
net_http_svr_mount_point_t net_http_svr_mount_point_find_child_by_path(net_http_svr_mount_point_t mount_point, const char * * inout_path) {
    const char * sep;
    const char * path = *inout_path;
    uint8_t path_in_tmp = 0;
    const char * last_found_path = path;
    net_http_svr_mount_point_t last_found = mount_point->m_processor ? mount_point : NULL;

    if (last_found && net_http_svr_mount_point_find_child_process_mount(&last_found, &path, &path_in_tmp) != 0) return NULL;
                                                               
    while((sep = strchr(path, '/'))) {
        if (sep > path) {
            mount_point = net_http_svr_mount_point_child_find_by_name_ex(mount_point, path, sep);
            if (mount_point == NULL) break;

            path = sep + 1;

            if (net_http_svr_mount_point_find_child_process_mount(&mount_point, &path, &path_in_tmp) != 0) return NULL;
            
            if (mount_point->m_processor) {
                last_found = mount_point;
                last_found_path = path;
            }
        }
        else {
            path = sep + 1;
            if (mount_point == last_found) last_found_path = path;
        }
    }
    
    if (mount_point && path[0]) {
        size_t path_len = strlen(path);
        mount_point = net_http_svr_mount_point_child_find_by_name_ex(mount_point, path, path + path_len);
        if (mount_point) {
            path += path_len;

            if (net_http_svr_mount_point_find_child_process_mount(&mount_point, &path, &path_in_tmp) != 0) return NULL;
            
            if (mount_point->m_processor) {
                last_found = mount_point;
                last_found_path = path;
            }
        }
    }

    *inout_path = last_found_path;
    return last_found;
}

void net_http_svr_mount_point_set_processor(net_http_svr_mount_point_t mp, void * processor_env, net_http_svr_processor_t processor) {
    if (mp->m_processor) {
        if (mp->m_processor->m_env_clear && mp->m_processor_env) {
            mp->m_processor->m_env_clear(mp->m_processor->m_ctx, mp->m_processor_env);
        }
        
        TAILQ_REMOVE(&mp->m_processor->m_mount_points, mp, m_next_for_processor);
    }

    mp->m_processor_env = processor_env;
    mp->m_processor = processor;
    
    if (mp->m_processor) {
        TAILQ_INSERT_TAIL(&mp->m_processor->m_mount_points, mp, m_next_for_processor);
    }
}

void net_http_svr_mount_point_clear_path(net_http_svr_mount_point_t mount_point) {
    while(TAILQ_EMPTY(&mount_point->m_childs) && mount_point->m_processor == NULL) {
        net_http_svr_mount_point_t p = mount_point->m_parent;
        if (p == NULL) break;
        net_http_svr_mount_point_free(mount_point);
        mount_point = p;
    }
}

net_http_svr_mount_point_t
net_http_svr_mount_point_mount(net_http_svr_mount_point_t from, const char * path, void * processor_env, net_http_svr_processor_t processor) {
    net_http_svr_protocol_t service = from->m_service;
    const char * sep;
    net_http_svr_mount_point_t mp = from;

    while((sep = strchr(path, '/'))) {
        if (sep > path) {
            net_http_svr_mount_point_t child_mp = NULL;
            child_mp = net_http_svr_mount_point_child_find_by_name_ex(mp, path, sep);
            if (child_mp == NULL) {
                child_mp = net_http_svr_mount_point_create(service, path, sep, mp, NULL, NULL);
                if (child_mp == NULL) {
                    CPE_ERROR(service->m_em, "http_svr: mount_point: mount: create mount point %s fail!", path);
                    return NULL;
                }
            }

            mp = child_mp;
        }
        
        path = sep + 1;
    }

    if (path[0]) {
        mp = net_http_svr_mount_point_create(service, path, path + strlen(path), mp, NULL, NULL);
        if (mp == NULL) {
            CPE_ERROR(service->m_em, "http_svr: mount_point: mount: create mount point %s fail!", path);
            return NULL;
        }
    }

    assert(mp);
    net_http_svr_mount_point_set_processor(mp, processor_env, processor);
    
    return mp;
}

int net_http_svr_mount_point_unmount(net_http_svr_mount_point_t from, const char * path) {
    net_http_svr_protocol_t service = from->m_service;
    net_http_svr_mount_point_t mp = from;

    mp = net_http_svr_mount_point_child_find_by_path_ex(from, path, path + strlen(path));
    if (mp == NULL) {
        CPE_ERROR(service->m_em, "http_svr: mount_point: unmount: point %s not exist!", path);
        return -1;
    }
    
    if (mp->m_processor == NULL) {
        CPE_ERROR(service->m_em, "http_svr: mount_point: unmount: point %s no explicit processor!", mp->m_name);
        return -1;
    }

    if (mp->m_parent == NULL) {
        /* net_http_svr_processor_t native_processor = net_http_svr_processor_native(mgr); */
        
        /* if (mp->m_processor != native_processor) { */
        /*     if (mp == mgr->m_current) { */
        /*         net_http_svr_mount_point_set_processor(mp, NULL, native_processor); */
        /*     } */
        /*     else if (mp == mgr->m_root) { */
        /*         char * d = cpe_str_mem_dup(mgr->m_alloc, ""); */
        /*         if (d == NULL) { */
        /*             CPE_ERROR(mgr->m_em, "net_http_svr_mount_point_unmount: dup root fail!"); */
        /*             return -1; */
        /*         } */
                
        /*         net_http_svr_mount_point_set_processor(mp, d, native_processor); */
        /*     } */
        /*     else { */
        /*         CPE_ERROR(mgr->m_em, "net_http_svr_mount_point_unmount: root mount point %s type unknown!", mp->m_name); */
        /*         return -1; */
        /*     } */
        /* } */
    }
    else {
        net_http_svr_mount_point_set_processor(mp, NULL, NULL);
        net_http_svr_mount_point_clear_path(mp);
    }

    return 0;
}

net_http_svr_mount_point_t
net_http_svr_mount_point_child_find_by_name_ex(net_http_svr_mount_point_t parent, const char * name, const char * name_end) {
    net_http_svr_mount_point_t child_mp = NULL;
        
    TAILQ_FOREACH(child_mp, &parent->m_childs, m_next_for_parent) {
        if (name_end - name == child_mp->m_name_len && memcmp(child_mp->m_name, name, child_mp->m_name_len) == 0) {
            return child_mp;
        }
    }

    return NULL;
}

net_http_svr_mount_point_t
net_http_svr_mount_point_child_find_by_path_ex(net_http_svr_mount_point_t parent, const char * path, const char * path_end) {
    const char * sep;
    
    while((sep = memchr(path, '/', path_end - path))) {
        if (sep > path) {
            parent = net_http_svr_mount_point_child_find_by_name_ex(parent, path, sep);
            if (parent == NULL) return NULL;
        }
        path = sep + 1;
    }

    if (path[0]) {
        parent = net_http_svr_mount_point_child_find_by_name_ex(parent, path, path_end);
    }
    
    return parent;
}
