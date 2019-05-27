set(net_driver_sock_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_sock)

file(GLOB net_driver_sock_source ${net_driver_sock_base}/src/*.c)

add_library(net_driver_sock STATIC ${net_driver_sock_source})

set_property(TARGET net_driver_sock PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_driver_sock_base}/include
  )