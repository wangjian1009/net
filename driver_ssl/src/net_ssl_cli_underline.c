#include <assert.h>
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ssl_cli_underline_i.h"

int net_ssl_cli_underline_init(net_endpoint_t base_underline) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_underline);
    undline->m_ssl_endpoint = NULL;
    return 0;
}

void net_ssl_cli_underline_fini(net_endpoint_t base_underline) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_underline);

    if (undline->m_ssl_endpoint) {
        assert(undline->m_ssl_endpoint->m_underline == base_underline);
        undline->m_ssl_endpoint->m_underline = NULL;
    }
    
    undline->m_ssl_endpoint = NULL;
}

int net_ssl_cli_underline_input(net_endpoint_t base_underline) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_underline);
    assert(undline->m_ssl_endpoint);
    assert(undline->m_ssl_endpoint->m_underline == base_underline);

    net_endpoint_t base_endpoint = net_endpoint_from_data(undline->m_ssl_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (net_endpoint_state(base_endpoint) == net_endpoint_state_connecting) {
        if (net_ssl_cli_endpoint_do_handshake(base_endpoint, endpoint) != 0) return -1;
    }

    if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return -1;


	int r;
	int all_result_flags = 0;

	uint32_t n_to_read = net_endpoint_buf_size(base_underline, net_ep_buf_read);
    CPE_ERROR(
        driver->m_em, "xxxx: %s: input %d",
        net_endpoint_dump(net_ssl_cli_driver_tmp_buffer(driver), base_underline), n_to_read);
    
	while(n_to_read) {
        n_to_read = 0;
	/* 	r = do_read(bev_ssl, n_to_read); */
	/* 	all_result_flags |= r; */

	/* 	if (r & (OP_BLOCKED|OP_ERR)) */
	/* 		break; */

	/* 	if (bev_ssl->bev.read_suspended) */
	/* 		break; */

	/* 	/\* Read all pending data.  This won't hit the network */
	/* 	 * again, and will (most importantly) put us in a state */
	/* 	 * where we don't need to read anything else until the */
	/* 	 * socket is readable again.  It'll potentially make us */
	/* 	 * overrun our read high-watermark (somewhat */
	/* 	 * regrettable).  The damage to the rate-limit has */
	/* 	 * already been done, since OpenSSL went and read a */
	/* 	 * whole SSL record anyway. *\/ */
	/* 	n_to_read = SSL_pending(bev_ssl->ssl); */

	/* 	/\* XXX This if statement is actually a bad bug, added to avoid */
	/* 	 * XXX a worse bug. */
	/* 	 * */
	/* 	 * The bad bug: It can potentially cause resource unfairness */
	/* 	 * by reading too much data from the underlying bufferevent; */
	/* 	 * it can potentially cause read looping if the underlying */
	/* 	 * bufferevent is a bufferevent_pair and deferred callbacks */
	/* 	 * aren't used. */
	/* 	 * */
	/* 	 * The worse bug: If we didn't do this, then we would */
	/* 	 * potentially not read any more from bev_ssl->underlying */
	/* 	 * until more data arrived there, which could lead to us */
	/* 	 * waiting forever. */
	/* 	 *\/ */
	/* 	if (!n_to_read && bev_ssl->underlying) */
	/* 		n_to_read = bytes_to_read(bev_ssl); */
	/* } */

	/* if (all_result_flags & OP_MADE_PROGRESS) { */
	/* 	struct bufferevent *bev = &bev_ssl->bev.bev; */

	/* 	bufferevent_trigger_nolock_(bev, EV_READ, 0); */
	}
    
    return 0;
}

int net_ssl_cli_underline_on_state_change(net_endpoint_t base_underline, net_endpoint_state_t from_state) {
    net_ssl_cli_undline_t undline = net_endpoint_protocol_data(base_underline);
    assert(undline->m_ssl_endpoint);
    assert(undline->m_ssl_endpoint->m_underline == base_underline);

    net_endpoint_t base_endpoint = net_endpoint_from_data(undline->m_ssl_endpoint);
    net_ssl_cli_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_ssl_cli_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    switch(net_endpoint_state(base_underline)) {
    case net_endpoint_state_resolving:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_resolving) != 0) return -1;
        break;
    case net_endpoint_state_connecting:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        break;
    case net_endpoint_state_established:
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        if (net_ssl_cli_endpoint_do_handshake(base_endpoint, endpoint) != 0) return -1;
        break;
    case net_endpoint_state_logic_error:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source(base_underline),
            net_endpoint_error_no(base_underline), net_endpoint_error_msg(base_underline));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) return -1;
        break;
    case net_endpoint_state_network_error:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source(base_underline),
            net_endpoint_error_no(base_underline), net_endpoint_error_msg(base_underline));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
        break;
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        net_endpoint_set_error(
            base_endpoint,
            net_endpoint_error_source_network,
            net_endpoint_network_errno_logic,
            "underline ep state error");
        return -1;
    }
    
    return 0;
}

net_protocol_t
net_ssl_cli_underline_protocol_create(net_schedule_t schedule, const char * name) {
    return net_protocol_create(
        schedule,
        name,
        /*protocol*/
        0, NULL, NULL,
        /*endpoint*/
        sizeof(struct net_ssl_cli_undline),
        net_ssl_cli_underline_init,
        net_ssl_cli_underline_fini,
        net_ssl_cli_underline_input,
        net_ssl_cli_underline_on_state_change,
        NULL);
}
