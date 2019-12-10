set(net_dns_udns_base ${CMAKE_CURRENT_LIST_DIR}/../../dns_udns)

file(GLOB net_dns_udns_source ${net_dns_udns_base}/src/*.c)

add_library(net_dns_udns STATIC ${net_dns_udns_source})

set_property(TARGET net_dns_udns PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils_sock/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../dns/include
  ${net_dns_udns_base}/include
  )
