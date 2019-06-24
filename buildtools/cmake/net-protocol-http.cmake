set(net_protocol_http_base ${CMAKE_CURRENT_LIST_DIR}/../../protocol_http)

file(GLOB net_protocol_http_source ${net_protocol_http_base}/src/*.c)

add_library(net_protocol_http STATIC ${net_protocol_http_source})

set_property(TARGET net_protocol_http PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/mbedtls/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_protocol_http_base}/include
  )
