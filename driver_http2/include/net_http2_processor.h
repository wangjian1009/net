#ifndef NET_HTTP2_PROCESSOR_H_INCLEDED
#define NET_HTTP2_PROCESSOR_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

enum net_http2_processor_state {
    net_http2_processor_state_init,          /*正在接受头部信息 */
    net_http2_processor_state_head_received, /*头部信息接受完整，响应没有发出 */
    net_http2_processor_state_established,   /*响应头已经发出，可以发送后续数据 */
    net_http2_processor_state_read_closed,   /*停止读 */
    net_http2_processor_state_write_closed,  /*停止写入 */
    net_http2_processor_state_closed,        /*已经关闭 */
};

net_http2_processor_state_t net_http2_processor_state(net_http2_processor_t processor);
net_http2_endpoint_t net_http2_processor_endpoint(net_http2_processor_t processor);
net_http2_stream_t net_http2_processor_stream(net_http2_processor_t processor);

const char * net_http2_processor_state_str(net_http2_processor_state_t state);

int net_http2_processor_add_res_head(net_http2_processor_t processor, const char * attr_name, const char * attr_value);

NET_END_DECL

#endif
