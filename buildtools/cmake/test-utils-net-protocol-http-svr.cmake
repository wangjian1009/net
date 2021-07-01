if (DEFINED net_protocol_http_svr_base)
  file(GLOB test_utils_net_protocol_http_svr_source ${net_protocol_http_svr_base}/test_utils/*.c)

  set(
    test_utils_net_protocol_http_svr_link_libraries
    net_protocol_http_svr test_utils_net_protocol_http test_utils_net_core)
  
  if(DEFINED net_driver_ssl_base)
    list(APPEND test_utils_net_protocol_http_svr_compile_definitions TESTS_UTILS_NET_PROTOCOL_HTTP_SVR_SSL=1)
    list(APPEND test_utils_net_protocol_http_svr_link_libraries net_driver_ssl)
  endif()
  
  add_library(test_utils_net_protocol_http_svr STATIC ${test_utils_net_protocol_http_svr_source})

  set_property(TARGET test_utils_net_protocol_http_svr PROPERTY COMPILE_DEFINITIONS
    ${test_utils_net_protocol_http_svr_compile_definitions})
  
  set_property(TARGET test_utils_net_protocol_http_svr PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${net_core_includes}
    ${net_core_base}/test_utils
    ${cpe_utils_base}/test_utils
    ${net_protocol_http_base}/include
    ${net_protocol_http_base}/test_utils
    ${net_protocol_http_svr_base}/include
    ${net_driver_ssl_base}/include
    )

  target_link_libraries(
    test_utils_net_protocol_http_svr INTERFACE ${test_utils_net_protocol_http_svr_link_libraries})
endif()
