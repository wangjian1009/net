file(GLOB test_utils_net_protocol_http_svr_source ${net_protocol_http_svr_base}/test_utils/*.c)

add_library(test_utils_net_protocol_http_svr STATIC ${test_utils_net_protocol_http_svr_source})

set_property(TARGET test_utils_net_protocol_http_svr PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${net_core_includes}
  ${net_core_base}/test_utils
  ${cpe_utils_base}/test_utils
  ${net_protocol_http_svr_base}/include
  )

target_link_libraries(test_utils_net_protocol_http_svr INTERFACE net_protocol_http_svr test_utils_net_core)
