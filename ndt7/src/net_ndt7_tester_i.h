#ifndef NET_NDT7_TESTER_I_H_INCLEDED
#define NET_NDT7_TESTER_I_H_INCLEDED
#include "net_ndt7_tester.h"
#include "net_ndt7_manage_i.h"

NET_BEGIN_DECL

struct net_ndt7_tester {
    net_ndt7_manage_t m_manager;
    uint32_t m_id;
    TAILQ_ENTRY(net_ndt7_tester) m_next;
    
    net_ndt7_test_type_t m_type;
    net_ndt7_tester_state_t m_state;
    cpe_url_t m_target;
    
    /**/
    union {
        struct {
            net_http_endpoint_t m_endpoint;
            net_http_req_t m_req;
        } m_query_target;
    } m_state_data;
    
    /*callback*/
    void * m_ctx;
    net_ndt7_tester_on_complete_fun_t m_on_complete;
    void (*m_ctx_free)(void *);
};

int net_ndt7_tester_query_target_start(net_ndt7_tester_t tester);
int net_ndt7_tester_download_start(net_ndt7_tester_t tester);
int net_ndt7_tester_upload_start(net_ndt7_tester_t tester);

int net_ndt7_tester_check_start_next_step(net_ndt7_tester_t tester);

NET_END_DECL

#endif
