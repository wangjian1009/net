set(net_http2_svr_base ${CMAKE_CURRENT_LIST_DIR}/../../http2_svr)

file(GLOB net_http2_svr_source ${net_http2_svr_base}/src/*.c)

add_library(net_http2_svr STATIC ${net_http2_svr_source})

set_property(TARGET net_http2_svr PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${openssl_base}/include
  ${nghttp2_base}/lib/includes
  ${CMAKE_CURRENT_LIST_DIR}/../custom/nghttp2
  ${net_http2_svr_base}/include
  )

add_dependencies(net_http2_svr ssl)
