#ifndef NET_HTTP2_UTILS_I_H_INCLEDED
#define NET_HTTP2_UTILS_I_H_INCLEDED
#include "net_http2_protocol_i.h"
#include "nghttp2/nghttp2.h"

#define OUTPUT_WOULDBLOCK_THRESHOLD (1 << 16)

#define MAKE_NV(NAME, VALUE, VALUELEN)                                         \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, VALUELEN,             \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

#define MAKE_NV2(NAME, VALUE)                                                  \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1,    \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

void net_http2_print_frame(write_stream_t ws, const nghttp2_frame *frame);

const char * net_http2_frame_type_str(nghttp2_frame_type frame_type);
const char * net_http2_error_code_str(nghttp2_error_code error_code);
const char * net_http2_settings_id_str(nghttp2_settings_id settings_id);
const char * net_nghttp2_headers_category_str(nghttp2_headers_category headers_category);
const char * net_nghttp2_flag_str(nghttp2_flag flag, uint8_t is_for_stream);

#endif
