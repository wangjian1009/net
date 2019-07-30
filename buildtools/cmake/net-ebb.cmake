set(net_ebb_base ${CMAKE_CURRENT_LIST_DIR}/../../ebb)

file(GLOB net_ebb_source ${net_ebb_base}/src/*.c)

add_library(net_ebb STATIC ${net_ebb_source})

set_property(TARGET net_ebb PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_ebb_base}/include
  )
