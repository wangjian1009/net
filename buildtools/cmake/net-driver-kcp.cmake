set(net_driver_kcp_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_kcp)

file(GLOB net_driver_kcp_source ${net_driver_kcp_base}/src/*.c)

add_library(net_driver_kcp STATIC ${net_driver_kcp_source})

set_property(TARGET net_driver_kcp PROPERTY INCLUDE_DIRECTORIES
  ${kcp_base}
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${net_core_base}/include
  ${net_driver_kcp_base}/include
  )

target_link_libraries(net_driver_kcp INTERFACE net_core kcp)
