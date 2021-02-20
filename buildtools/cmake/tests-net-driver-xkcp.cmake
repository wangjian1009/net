file(GLOB test_net_driver_xkcp_source ${net_driver_xkcp_base}/tests/*.c)

add_executable(tests_net_driver_xkcp ${test_net_driver_xkcp_source})

set_property(TARGET tests_net_driver_xkcp PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${xkcp_base}
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_base}/test_utils
  ${net_core_base}/include
  ${net_core_base}/test_utils
  ${net_driver_xkcp_base}/include
  ${net_driver_xkcp_base}/src
  )

set(tests_net_driver_xkcp_libraries
  test_utils_net_core
  net_driver_xkcp)

set_property(TARGET tests_net_driver_xkcp PROPERTY LINK_LIBRARIES ${tests_net_driver_xkcp_libraries})

add_test(NAME net-xkcp COMMAND tests_net_driver_xkcp)
