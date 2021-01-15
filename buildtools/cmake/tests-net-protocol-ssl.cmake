file(GLOB test_net_protocol_ssl_source ${net_protocol_ssl_base}/tests/*.c)

add_executable(tests_net_protocol_ssl ${test_net_protocol_ssl_source})

set_property(TARGET tests_net_protocol_ssl PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_base}/test_utils
  ${cpe_utils_yaml_base}/include
  ${net_core_base}/include
  ${net_core_base}/test_utils
  ${net_protocol_ssl_base}/include
  )

set(tests_net_protocol_ssl_libraries
  test_utils_cpe_utils
  net_protocol_ssl net_core cpe_utils_yaml cpe_utils_sock cpe_utils cpe_pal pcre2 cmocka)

if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64)
  set(tests_net_protocol_ssl_libraries ${tests_net_protocol_ssl_libraries} m)
endif()

set_property(TARGET tests_net_protocol_ssl PROPERTY LINK_LIBRARIES ${tests_net_protocol_ssl_libraries})

add_test(NAME net-ssl COMMAND tests_net_protocol_ssl)
