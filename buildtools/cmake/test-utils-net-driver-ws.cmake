file(GLOB test_utils_net_driver_ws_source ${net_driver_ws_base}/test_utils/*.c)

add_library(test_utils_net_driver_ws STATIC ${test_utils_net_driver_ws_source})

set_property(TARGET test_utils_net_driver_ws PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${net_core_includes}
  ${cpe_utils_base}/test_utils
  ${net_driver_ws_base}/include
  )
