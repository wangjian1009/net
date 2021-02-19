#ifndef NET_SMUX_STREAM_I_H_INCLEDED
#define NET_SMUX_STREAM_I_H_INCLEDED
#include "net_smux_stream.h"
#include "net_smux_session_i.h"

NET_BEGIN_DECL

struct net_smux_stream {
    net_smux_session_t m_session;
    struct cpe_hash_entry m_hh_for_session;
    uint32_t m_stream_id;

    uint32_t m_frame_size;
    
	uint32_t m_num_read;      /* number of consumed bytes */
	uint32_t m_num_written;   /* count num of bytes written */
	uint32_t m_incr;          /* counting for sending */

	/* UPD command */
	uint32_t m_peer_consumed; /* num of bytes the peer has consumed */
	uint32_t m_peer_window;   /* peer window, initialized to 256KB, updated by peer */

    /*write callback*/
    void * m_write_ctx;
    net_smux_stream_on_write_resume_fun_t m_on_write_resume;
    void (*m_write_ctx_free)(void *);
    
    /*read callback*/
    void * m_read_ctx;
    net_smux_stream_on_recv_fun_t m_on_recv;
    void (*m_read_ctx_free)(void *);
};

net_smux_stream_t
net_smux_stream_create(
    net_smux_session_t session, uint32_t stream_id, uint32_t frame_size);

void net_smux_stream_free(net_smux_stream_t stream);

void net_smux_stream_free_all(net_smux_session_t session);

void net_smux_stream_update_pear(net_smux_stream_t stream, uint32_t consumed, uint32_t window);

void net_smux_stream_print(write_stream_t ws, net_smux_stream_t stream);
const char * net_smux_stream_dump(mem_buffer_t buffer, net_smux_stream_t stream);

int net_smux_stream_eq(net_smux_stream_t l, net_smux_stream_t r, void * user_data);
uint32_t net_smux_stream_hash(net_smux_stream_t o, void * user_data);

NET_END_DECL

#endif
