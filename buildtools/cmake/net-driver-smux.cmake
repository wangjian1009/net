set(net_driver_smux_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_smux)

file(GLOB net_driver_smux_source ${net_driver_smux_base}/src/*.c)

add_library(net_driver_smux STATIC ${net_driver_smux_source})

set_property(TARGET net_driver_smux PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_driver_smux_base}/include
  )
