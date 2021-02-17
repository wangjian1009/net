set(net_protocol_smux_base ${CMAKE_CURRENT_LIST_DIR}/../../protocol_smux)

file(GLOB net_protocol_smux_source ${net_protocol_smux_base}/src/*.c)

add_library(net_protocol_smux STATIC ${net_protocol_smux_source})

set_property(TARGET net_protocol_smux PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_protocol_smux_base}/include
  )
