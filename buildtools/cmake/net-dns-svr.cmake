set(net_dns_svr_base ${CMAKE_CURRENT_LIST_DIR}/../../dns_svr)

file(GLOB net_dns_svr_source ${net_dns_svr_base}/src/*.c)

add_library(net_dns_svr STATIC ${net_dns_svr_source})

set_property(TARGET net_dns_svr PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_dns_svr_base}/include
  )
