set(net_statistics_base ${CMAKE_CURRENT_LIST_DIR}/../../statistics)

file(GLOB net_statistics_source ${net_statistics_base}/src/*.c)

add_library(net_statistics STATIC ${net_statistics_source})

set_property(TARGET net_statistics PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${net_statistics_base}/include
  )
