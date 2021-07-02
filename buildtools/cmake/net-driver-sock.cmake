set(net_driver_sock_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_sock)

file(GLOB net_driver_sock_source ${net_driver_sock_base}/src/*.c)

set(net_driver_sock_includes
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_sock_base}/include
  ${net_core_base}/include
  ${net_driver_sock_base}/include)

set(net_driver_sock_link_libraries net_core cpe_utils_sock)
  
if (OS_NAME MATCHES "mingw")
  list(APPEND net_driver_sock_includes ${unixem_base}/include)
  list(APPEND net_driver_sock_link_libraries unixem)
endif()

add_library(net_driver_sock STATIC ${net_driver_sock_source})

target_link_libraries(net_driver_sock INTERFACE ${net_driver_sock_link_libraries})

set_property(TARGET net_driver_sock PROPERTY INCLUDE_DIRECTORIES ${net_driver_sock_includes})
