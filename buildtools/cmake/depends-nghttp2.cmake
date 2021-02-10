set(nghttp2_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/nghttp2)
set(nghttp2_custom ${CMAKE_CURRENT_LIST_DIR}/../custom/nghttp2)

set(nghttp2_source
  ${nghttp2_base}/lib/nghttp2_pq.c
  ${nghttp2_base}/lib/nghttp2_map.c
  ${nghttp2_base}/lib/nghttp2_queue.c
  ${nghttp2_base}/lib/nghttp2_frame.c
  ${nghttp2_base}/lib/nghttp2_buf.c
  ${nghttp2_base}/lib/nghttp2_stream.c
  ${nghttp2_base}/lib/nghttp2_outbound_item.c
  ${nghttp2_base}/lib/nghttp2_session.c
  ${nghttp2_base}/lib/nghttp2_submit.c
  ${nghttp2_base}/lib/nghttp2_helper.c
  ${nghttp2_base}/lib/nghttp2_npn.c
  ${nghttp2_base}/lib/nghttp2_hd.c
  ${nghttp2_base}/lib/nghttp2_hd_huffman.c
  ${nghttp2_base}/lib/nghttp2_hd_huffman_data.c
  ${nghttp2_base}/lib/nghttp2_version.c
  ${nghttp2_base}/lib/nghttp2_priority_spec.c
  ${nghttp2_base}/lib/nghttp2_option.c
  ${nghttp2_base}/lib/nghttp2_callbacks.c
  ${nghttp2_base}/lib/nghttp2_mem.c
  ${nghttp2_base}/lib/nghttp2_http.c
  ${nghttp2_base}/lib/nghttp2_rcbuf.c
  ${nghttp2_base}/lib/nghttp2_debug.c
  ${nghttp2_base}/lib/nghttp2_ksl.c
)

if (MSVC)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
set(nghttp2_compile_options
  -Wno-implicit-function-declaration
  )  
endif ()
    
set(nghttp2_compile_definitions
  NGHTTP2_STATICLIB
  NOTHREADS=1)

if (ANDROID)
  set(nghttp2_compile_definitions ${nghttp2_compile_definitions} HAVE_NETINET_IN_H)
endif ()

add_library(nghttp2 STATIC ${nghttp2_source})
set_property(TARGET nghttp2 PROPERTY COMPILE_OPTIONS ${nghttp2_compile_options})
set_property(TARGET nghttp2 PROPERTY COMPILE_DEFINITIONS ${nghttp2_compile_definitions})

set_property(TARGET nghttp2 PROPERTY INCLUDE_DIRECTORIES
  ${nghttp2_base}/lib/includes
  ${nghttp2_custom}
  )
