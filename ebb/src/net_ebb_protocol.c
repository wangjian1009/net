#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_ebb_protocol_i.h"
#include "net_ebb_connection_i.h"
#include "net_ebb_request_i.h"
#include "net_ebb_request_header_i.h"
#include "net_ebb_mount_point_i.h"
#include "net_ebb_processor_i.h"
#include "net_ebb_response_i.h"

static int net_ebb_protocol_init(net_protocol_t protocol);
static void net_ebb_protocol_fini(net_protocol_t protocol);

net_ebb_protocol_t
net_ebb_protocol_create(net_schedule_t schedule, const char * protocol_name) {
    net_protocol_t protocol =
        net_protocol_create(
            schedule,
            protocol_name,
            /*protocol*/
            sizeof(struct net_ebb_protocol),
            net_ebb_protocol_init,
            net_ebb_protocol_fini,
            /*endpoint*/
            sizeof(struct net_ebb_connection),
            net_ebb_connection_init,
            net_ebb_connection_fini,
            net_ebb_connection_input,
            NULL,
            NULL);
    if (protocol == NULL) {
        CPE_ERROR(net_schedule_em(schedule), "ebb: %s: create protocol fail!", protocol_name);
        return NULL;
    }
    net_protocol_set_debug(protocol, 2);

    net_ebb_protocol_t service = net_protocol_data(protocol);
    
    return service;
}

void net_ebb_protocol_free(net_ebb_protocol_t service) {
    net_protocol_free(net_protocol_from_data(service));
}

net_protocol_t net_ebb_protocol_to_protocol(net_ebb_protocol_t service) {
    return net_protocol_from_data(service);
}

net_ebb_mount_point_t net_ebb_protocol_root(net_ebb_protocol_t service) {
    return service->m_root;
}

mem_buffer_t net_ebb_protocol_tmp_buffer(net_ebb_protocol_t service) {
    net_protocol_t protocol = net_ebb_protocol_to_protocol(service);
    return net_schedule_tmp_buffer(net_protocol_schedule(protocol));
}

static int net_ebb_protocol_init(net_protocol_t protocol) {
    net_ebb_protocol_t service = net_protocol_data(protocol);

    service->m_em = net_schedule_em(net_protocol_schedule(protocol));
    service->m_alloc = net_schedule_allocrator(net_protocol_schedule(protocol));
    service->m_cfg_connection_timeout_ms = 30 * 1000;

    TAILQ_INIT(&service->m_processors);
    service->m_root = NULL;
    TAILQ_INIT(&service->m_connections);
    service->m_request_sz = 0;
    
    service->m_max_request_id = 0;
    TAILQ_INIT(&service->m_free_requests);
    TAILQ_INIT(&service->m_free_request_headers);
    TAILQ_INIT(&service->m_free_mount_points);
    TAILQ_INIT(&service->m_free_responses);

    mem_buffer_init(&service->m_data_buffer, NULL);
    mem_buffer_init(&service->m_search_path_buffer, NULL);

    net_ebb_processor_t dft_processor = net_ebb_processor_create_dft(service);
    if (dft_processor == NULL) {
        CPE_ERROR(service->m_em, "ebb: %s: create dft processor fail!", net_protocol_name(protocol));
        return -1;
    }
    
    const char * root_mp_name = "root";
    service->m_root = net_ebb_mount_point_create(
        service, root_mp_name, root_mp_name + strlen(root_mp_name), NULL, NULL, dft_processor);
    if (service->m_root == NULL) {
        CPE_ERROR(service->m_em, "ebb: %s: create root mount point fail!", net_protocol_name(protocol));
        net_ebb_processor_free(dft_processor);
        return -1;
    }
    
    return 0;
}

static void net_ebb_protocol_fini(net_protocol_t protocol) {
    net_ebb_protocol_t service = net_protocol_data(protocol);
    assert(TAILQ_EMPTY(&service->m_connections));

    if (service->m_root) {
        net_ebb_mount_point_free(service->m_root);
        service->m_root = NULL;
    }
    
    while(!TAILQ_EMPTY(&service->m_processors)) {
        net_ebb_processor_free(TAILQ_FIRST(&service->m_processors));
    }
    
    while(!TAILQ_EMPTY(&service->m_free_requests)) {
        net_ebb_request_real_free(TAILQ_FIRST(&service->m_free_requests));
    }

    while(!TAILQ_EMPTY(&service->m_free_request_headers)) {
        net_ebb_request_header_real_free(TAILQ_FIRST(&service->m_free_request_headers));
    }

    while(!TAILQ_EMPTY(&service->m_free_responses)) {
        net_ebb_response_real_free(TAILQ_FIRST(&service->m_free_responses));
    }
    
    while (!TAILQ_EMPTY(&service->m_free_mount_points)) {
        net_ebb_mount_point_real_free(TAILQ_FIRST(&service->m_free_mount_points));
    }

    mem_buffer_clear(&service->m_data_buffer);
    mem_buffer_clear(&service->m_search_path_buffer);
}

struct {
    const char * m_mine_type;
    const char * m_postfix;
} s_common_mine_type_defs[] = {
    { "application/msword", "doc" }
    , { "application/pdf", "pdf" }
    , { "application/rtf", "rtf" }
    , { "application/vnd.ms-excel", "xls" }
    , { "application/vnd.ms-powerpoint", "ppt" }
    , { "application/x-rar-compressed", "rar" }
    , { "application/x-shockwave-flash", "swf" }
    , { "application/zip", "zip" }
    , { "audio/midi", "mid" }     //midi kar
    , { "audio/mpeg", "mp3" }
    , { "audio/ogg", "ogg" }
    , { "audio/x-m4a", "m4a" }
    , { "audio/x-realaudio", "ra" }
    , { "image/gif", "gif" }
    , { "image/jpeg", "jpeg" }
    , { "image/jpeg", "jpg" }
    , { "image/png", "png" }
    , { "image/tiff", "tif" }
    , { "image/tiff", "tiff" }
    , { "image/vnd.wap.wbmp", "wbmp" }
    , { "image/x-icon", "ico" }
    , { "image/x-jng", "jng" }
    , { "image/x-ms-bmp", "bmp" }
    , { "image/svg+xml", "svg" }
    , { "image/svg+xml", "svgz" }
    , { "image/webp", "webp" }
    , { "text/css", "css" }
    , { "text/html", "html" }
    , { "text/html", "tml" }
    , { "text/html", "shtml" }
    , { "text/plain", "txt" }
    , { "text/xml", "xml" }
    , { "video/3gpp", "3gpp" }
    , { "video/3gpp", "3gp" }
    , { "video/mp4", "mp4" }
    , { "video/mpeg", "mpeg" }
    , { "video/mpeg", "mpg" }
    , { "video/quicktime", "mov" }
    , { "video/webm", "webm" }
    , { "video/x-flv", "flv" }
    , { "video/x-m4v", "m4v" }
    , { "video/x-ms-wmv", "wmv" }
    , { "video/x-msvideo", "avi" }
};

const char * net_ebb_protocol_mine_from_postfix(net_ebb_protocol_t service, const char * postfix) {
    uint16_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(s_common_mine_type_defs); ++i) {
        if (strcasecmp(postfix, s_common_mine_type_defs[i].m_postfix) == 0) {
            return s_common_mine_type_defs[i].m_mine_type;
        }
    }
    
    return NULL;
}
