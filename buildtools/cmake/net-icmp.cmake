set(net_icmp_base ${CMAKE_CURRENT_LIST_DIR}/../../icmp)

file(GLOB net_icmp_source ${net_icmp_base}/src/*.c)

add_library(net_icmp STATIC ${net_icmp_source})

set_property(TARGET net_icmp PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_icmp_base}/include
  )
