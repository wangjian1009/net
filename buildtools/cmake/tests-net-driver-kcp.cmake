file(GLOB test_net_driver_kcp_source ${net_driver_kcp_base}/tests/*.c)

add_executable(tests_net_driver_kcp ${test_net_driver_kcp_source})

set_property(TARGET tests_net_driver_kcp PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${kcp_base}
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_base}/test_utils
  ${net_core_base}/include
  ${net_core_base}/test_utils
  ${net_protocol_smux_base}/include
  ${net_driver_kcp_base}/include
  ${net_driver_kcp_base}/src
  )

set(tests_net_driver_kcp_libraries
  test_utils_net_core
  net_driver_kcp)

set_property(TARGET tests_net_driver_kcp PROPERTY LINK_LIBRARIES ${tests_net_driver_kcp_libraries})

add_test(NAME net-kcp COMMAND tests_net_driver_kcp)
