#ifndef NET_SMUX_STREAM_H_INCLEDED
#define NET_SMUX_STREAM_H_INCLEDED
#include "net_smux_types.h"

NET_BEGIN_DECL

enum net_smux_stream_write_error {
    net_smux_stream_write_error_internal = -1,
    net_smux_stream_write_error_wouldblock = -2,
};

uint32_t net_smux_stream_id(net_smux_stream_t stream);
net_smux_session_t net_smux_stream_session(net_smux_stream_t stream);

/**/
void net_smux_stream_close_and_free(net_smux_stream_t stream);

/*写入处理 */
int net_smux_stream_write(net_smux_stream_t stream, void const * data, uint32_t data_len);

typedef void (*net_smux_stream_on_write_resume_fun_t)(void * ctx, net_smux_stream_t stream);

void net_smux_stream_set_writer(
    net_smux_stream_t stream,
    void * write_ctx,
    net_smux_stream_on_write_resume_fun_t on_write_resume,
    void (*write_ctx_free)(void *));

void net_smux_stream_clear_writer(net_smux_stream_t stream);

void * net_smux_stream_writer_ctx(net_smux_stream_t stream);

/*响应处理 */
typedef int (*net_smux_stream_on_recv_fun_t)(
    void * ctx, net_smux_stream_t stream, void const * data, uint32_t data_len);

void net_smux_stream_read_resume(net_smux_stream_t stream);

void net_smux_stream_set_reader(
    net_smux_stream_t stream,
    void * read_ctx,
    net_smux_stream_on_recv_fun_t on_recv,
    void (*read_ctx_free)(void *));

void net_smux_stream_clear_reader(net_smux_stream_t stream);

void * net_smux_stream_reader_ctx(net_smux_stream_t stream);

NET_END_DECL

#endif
