set(net_protocol_ssl_base ${CMAKE_CURRENT_LIST_DIR}/../../protocol_ssl)

file(GLOB net_protocol_ssl_source ${net_protocol_ssl_base}/src/*.c)

add_library(net_protocol_ssl STATIC ${net_protocol_ssl_source})

set_property(TARGET net_protocol_ssl PROPERTY INCLUDE_DIRECTORIES
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${net_core_base}/include
  ${net_protocol_ssl_base}/include
  )
