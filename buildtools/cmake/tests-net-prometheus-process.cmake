file(GLOB test_net_prometheus_process_source ${net_prometheus_process_base}/tests/*.c)

add_executable(tests_net_prometheus_process ${test_net_prometheus_process_source})

set_property(TARGET tests_net_prometheus_process PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_base}/test_utils
  ${cpe_vfs_base}/include
  ${cpe_vfs_base}/test_utils
  ${net_prometheus_base}/include
  ${net_prometheus_process_base}/include
  ${net_prometheus_process_base}/src
  )

set(tests_net_prometheus_process_libraries net_prometheus_process test_utils_cpe_utils test_utils_cpe_vfs cmocka)

set_property(TARGET tests_net_prometheus_process PROPERTY LINK_LIBRARIES ${tests_net_prometheus_process_libraries})

add_test(NAME prometheus_process COMMAND tests_net_prometheus_process)
