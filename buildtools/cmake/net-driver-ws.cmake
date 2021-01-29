set(net_driver_ws_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_ws)

file(GLOB net_driver_ws_source ${net_driver_ws_base}/src/*.c)

add_library(net_driver_ws STATIC ${net_driver_ws_source})

set_property(TARGET net_driver_ws PROPERTY INCLUDE_DIRECTORIES
  ${wslay_base}/include
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${net_core_base}/include
  ${net_driver_ws_base}/include
  )
