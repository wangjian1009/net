set(net_ndt_base ${CMAKE_CURRENT_LIST_DIR}/../../ndt)

file(GLOB net_ndt_source ${net_ndt_base}/src/*.c)

add_library(net_ndt STATIC ${net_ndt_source})

set_property(TARGET net_ndt PROPERTY INCLUDE_DIRECTORIES
  ${cpe_pal_base}/../../../cpe/pal/include
  ${cpe_utils_base}/../../../cpe/utils/include
  ${net_core_bas}/include
  ${net_ndt_base}/include
  )

target_link_libraries(net_ndt INTERFACE net_core)
