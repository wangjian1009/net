set(net_dns_base ${CMAKE_CURRENT_LIST_DIR}/../../dns/tests)

file(GLOB net_dns_source ${net_dns_base}/*.c)

add_executable(tests_net_dns ${net_dns_source})

set_property(TARGET tests_net_dns PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_BINARY_DIR}/check
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/depends/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils_sock/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../dns/include
  ${net_dns_base}/include
  )

set_property(TARGET tests_net_dns PROPERTY LINK_LIBRARIES
    net_dns net_core cpe_utils_sock cpe_utils cpe_pal pcre2 check)

add_test(NAME net-dns COMMAND tests_net_dns)
