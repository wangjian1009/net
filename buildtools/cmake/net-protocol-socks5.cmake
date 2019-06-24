set(net_protocol_socks5_base ${CMAKE_CURRENT_LIST_DIR}/../../protocol_socks5)

file(GLOB net_protocol_socks5_source ${net_protocol_socks5_base}/src/*.c)

add_library(net_protocol_socks5 STATIC ${net_protocol_socks5_source})

set_property(TARGET net_protocol_socks5 PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_protocol_socks5_base}/include
  )
