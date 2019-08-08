set(net_dns_base ${CMAKE_CURRENT_LIST_DIR}/../../dns/tests)

file(GLOB net_dns_source ${net_dns_base}/*.c)

add_executable(tests_net_dns ${net_dns_source})

set_property(TARGET tests_net_dns PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_BINARY_DIR}/check
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/depends/yaml/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils_yaml/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../dns/include
  ${net_dns_base}/include
  )

set(tests_net_dns_libraries
  net_dns net_core cpe_utils_sock cpe_utils_yaml cpe_utils cpe_pal yaml pcre2 check)
if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64)
  set(tests_net_dns_libraries ${tests_net_dns_libraries} m)
endif()

set_property(TARGET tests_net_dns PROPERTY LINK_LIBRARIES ${tests_net_dns_libraries})

add_test(NAME net-dns COMMAND tests_net_dns)
