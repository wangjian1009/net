set(net_protocol_ws_base ${CMAKE_CURRENT_LIST_DIR}/../../protocol_ws)

file(GLOB net_protocol_ws_source ${net_protocol_ws_base}/src/*.c)

add_library(net_protocol_ws STATIC ${net_protocol_ws_source})

set_property(TARGET net_protocol_ws PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/wslay/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../protocol_http/include
  ${net_protocol_ws_base}/include
  )
