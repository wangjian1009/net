file(GLOB test_net_prometheus_source ${net_prometheus_base}/tests/*.c)

add_executable(tests_net_prometheus ${test_net_prometheus_source})

set_property(TARGET tests_net_prometheus PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_base}/test_utils
  ${net_prometheus_base}/include
  ${net_prometheus_base}/src
  )

set(tests_net_prometheus_libraries net_prometheus test_utils_cpe_utils cmocka)

set_property(TARGET tests_net_prometheus PROPERTY LINK_LIBRARIES ${tests_net_prometheus_libraries})

add_test(NAME prometheus COMMAND tests_net_prometheus)
