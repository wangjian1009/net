set(net_driver_xkcp_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_xkcp)

file(GLOB net_driver_xkcp_source ${net_driver_xkcp_base}/src/*.c)

add_library(net_driver_xkcp STATIC ${net_driver_xkcp_source})

set_property(TARGET net_driver_xkcp PROPERTY INCLUDE_DIRECTORIES
  ${kcp_base}
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${net_core_base}/include
  ${net_driver_xkcp_base}/include
  )

target_link_libraries(net_driver_xkcp INTERFACE net_core kcp)
