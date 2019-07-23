set(net_ping_base ${CMAKE_CURRENT_LIST_DIR}/../../ping)

file(GLOB net_ping_source ${net_ping_base}/src/*.c)

add_library(net_ping STATIC ${net_ping_source})

set_property(TARGET net_ping PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_ping_base}/include
  )
