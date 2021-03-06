#ifndef URL_RUNNER_H_INCLEDED
#define URL_RUNNER_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_trans_system.h"
#include "net_http_types.h"
#include "net_system.h"
#include "net_dns_system.h"

struct ev_loop;
struct ev_signal;
typedef struct url_runner * url_runner_t;
typedef enum url_runner_mode url_runner_mode_t;

enum url_runner_mode {
    url_runner_mode_init,
    url_runner_mode_internal,
    url_runner_mode_libcurl,
};

struct url_runner {
    mem_allocrator_t m_alloc;
    FILE * m_log_file;
    struct error_monitor m_em_buf;
    error_monitor_t m_em;

    FILE * m_output;
    
    /*net*/
    struct ev_loop * m_event_base;
    uint8_t m_sig_event_count;
    uint8_t m_sig_event_capacity;
    struct ev_signal * m_sig_events;

    net_schedule_t m_net_schedule;
    net_dns_manage_t m_dns_manage;
    net_driver_t m_net_driver;
    
    url_runner_mode_t m_mode;
    union {
        struct {
            net_http_protocol_t m_http_protocol;
            net_http_endpoint_t m_http_endpoint;
            net_http_req_t m_http_req;
            net_driver_t m_ssl_driver;
        } m_internal;
        struct {
            net_trans_manage_t m_trans_mgr;
            net_trans_task_t m_trans_task;
        } m_libcurl;
    };
};

url_runner_t url_runner_create(mem_allocrator_t alloc);
void url_runner_free(url_runner_t runner);

/*主要操作 */
int url_runner_init_net(url_runner_t runner);
int url_runner_init_dns(url_runner_t runner);
int url_runner_set_mode(url_runner_t runner, url_runner_mode_t mode);
int url_runner_start(
    url_runner_t runner,
    const char * method,
    const char * url,
    const char * header[], uint16_t header_count,
    const char * body);

void url_runner_loop_run(url_runner_t runner);
void url_runner_loop_break(url_runner_t runner);
int url_runner_init_stop_sig(url_runner_t runner, int sig);

/*内部协议实现 */
int url_runner_internal_init(url_runner_t runner);
void url_runner_internal_fini(url_runner_t runner);
int url_runner_internal_start(
    url_runner_t runner,
    const char * method, const char * url,
    const char * header[], uint16_t header_count, const char * body);

/*libcurl实现 */
int url_runner_libcurl_init(url_runner_t runner);
void url_runner_libcurl_fini(url_runner_t runner);
int url_runner_libcurl_start(
    url_runner_t runner,
    const char * method, const char * url,
    const char * header[], uint16_t header_count, const char * body);

#endif
