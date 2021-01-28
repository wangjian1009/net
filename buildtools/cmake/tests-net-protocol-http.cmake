file(GLOB test_net_protocol_http_source ${net_protocol_http_base}/tests/*.c)

add_executable(tests_net_protocol_http ${test_net_protocol_http_source})

set_property(TARGET tests_net_protocol_http PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${openssl_base}/include
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_base}/test_utils
  ${net_core_base}/include
  ${net_core_base}/test_utils
  ${net_protocol_http_base}/include
  ${net_protocol_http_base}/src
  )

set(tests_net_protocol_http_libraries
  test_utils_cpe_utils test_utils_net_core
  net_protocol_http net_core  cpe_utils_sock cpe_utils cpe_pal
  pcre2 cmocka)

if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64)
  set(tests_net_protocol_http_libraries ${tests_net_protocol_http_libraries} m)
endif()

set_property(TARGET tests_net_protocol_http PROPERTY LINK_LIBRARIES ${tests_net_protocol_http_libraries})

add_test(NAME net-protocol-http COMMAND tests_net_protocol_http)
