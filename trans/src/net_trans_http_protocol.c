#include "assert.h"
#include "net_protocol.h"
#include "net_http_protocol.h"
#include "net_trans_http_protocol_i.h"
#include "net_trans_http_endpoint_i.h"

static int net_trans_http_protocol_init(net_http_protocol_t protocol);
static void net_trans_http_protocol_fini(net_http_protocol_t protocol);

net_trans_http_protocol_t net_trans_http_protocol_create(net_trans_manage_t manage) {
    net_http_protocol_t protocol = net_http_protocol_find(manage->m_schedule, "trans-http");
    if (protocol == NULL) {
        protocol =
            net_http_protocol_create(
                manage->m_schedule,
                "trans-http",
                /*protocol*/
                sizeof(struct net_trans_http_protocol),
                net_trans_http_protocol_init,
                net_trans_http_protocol_fini,
                /*endpoint*/
                sizeof(struct net_trans_http_endpoint),
                net_trans_http_endpoint_init,
                net_trans_http_endpoint_fini,
                NULL/*endpoint_input_for_upgraded*/,
                net_trans_http_endpoint_on_state_change);
        if (protocol == NULL) {
            return NULL;
        }
    }
    
    net_trans_http_protocol_t trans_protocol = net_http_protocol_data(protocol);
    trans_protocol->m_ref_count++;
    return trans_protocol;
}

void net_trans_http_protocol_free(net_trans_http_protocol_t trans_protocol) {
    assert(trans_protocol->m_ref_count > 0);
    trans_protocol->m_ref_count--;
    if (trans_protocol->m_ref_count == 0) {
        net_http_protocol_free(net_http_protocol_from_data(trans_protocol));
    }
}

static int net_trans_http_protocol_init(net_http_protocol_t protocol) {
    net_trans_http_protocol_t trans_protocol = net_http_protocol_data(protocol);
    trans_protocol->m_ref_count = 0;
    return 0;
}

static void net_trans_http_protocol_fini(net_http_protocol_t protocol) {
}
