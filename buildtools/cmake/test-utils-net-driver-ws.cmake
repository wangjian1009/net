file(GLOB test_utils_net_driver_ws_source ${net_driver_ws_base}/test_utils/*.c)

add_library(test_utils_net_driver_ws STATIC ${test_utils_net_driver_ws_source})

set_property(TARGET test_utils_net_driver_ws PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${cpe_utils_base}/test_utils
  ${net_core_includes}
  ${net_core_base}/test_utils
  ${net_driver_ws_base}/include
  ${net_driver_ssl_base}/include
  )

target_link_libraries(
  test_utils_net_driver_ws INTERFACE
  net_driver_ws test_utils_net_core net_driver_ssl)
