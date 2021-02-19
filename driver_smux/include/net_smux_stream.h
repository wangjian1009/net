#ifndef NET_SMUX_STREAM_H_INCLEDED
#define NET_SMUX_STREAM_H_INCLEDED
#include "net_smux_types.h"

NET_BEGIN_DECL

enum net_smux_stream_state {
    net_smux_stream_state_init,
    net_smux_stream_state_closed,
};

uint32_t net_smux_stream_id(net_smux_stream_t stream);
net_smux_session_t net_smux_stream_session(net_smux_stream_t stream);
net_smux_stream_state_t net_smux_stream_state(net_smux_stream_t stream);

/**/
void net_smux_stream_close_and_free(net_smux_stream_t stream);

/**/
int net_smux_stream_write(net_smux_stream_t stream, void const * data, uint32_t data_len);
    
/*响应处理 */
typedef int (*net_smux_stream_on_recv_fun_t)(void * ctx, net_smux_stream_t stream, void const * data, uint32_t data_len);

void net_smux_stream_set_reader(
    net_smux_stream_t stream,
    void * read_ctx,
    net_smux_stream_on_recv_fun_t on_recv,
    void (*read_ctx_free)(void *));

void net_smux_stream_clear_reader(net_smux_stream_t req);

void * net_smux_stream_reader_ctx(net_smux_stream_t req);

/*utils*/
const char * net_smux_stream_state_str(net_smux_stream_state_t state);

NET_END_DECL

#endif
