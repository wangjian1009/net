set(net_router_base ${CMAKE_CURRENT_LIST_DIR}/../../router)

file(GLOB net_router_source ${net_router_base}/src/*.c)

add_library(net_router STATIC ${net_router_source})

set_property(TARGET net_router PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_router_base}/include
  )
