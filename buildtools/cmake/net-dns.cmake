set(net_dns_base ${CMAKE_CURRENT_LIST_DIR}/../../dns)

file(GLOB net_dns_source ${net_dns_base}/src/*.c)

add_library(net_dns STATIC ${net_dns_source})

set_property(TARGET net_dns PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_dns_base}/include
  )
