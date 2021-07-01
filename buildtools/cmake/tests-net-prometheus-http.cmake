if(DEFINED net_prometheus_http_base)
  file(GLOB test_net_prometheus_http_source ${net_prometheus_http_base}/tests/*.c)

  add_executable(tests_net_prometheus_http ${test_net_prometheus_http_source})

  set_property(TARGET tests_net_prometheus_http PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_base}/test_utils
    ${net_core_base}/include
    ${net_core_base}/test_utils
    ${net_protocol_http_base}/include
    ${net_protocol_http_base}/test_utils
    ${net_protocol_http_svr_base}/include
    ${net_protocol_http_svr_base}/test_utils
    ${net_prometheus_base}/include
    ${net_prometheus_http_base}/include
    )

  set(tests_net_prometheus_http_libraries
    net_prometheus_http test_utils_net_protocol_http_svr test_utils_cpe_utils)

  set_property(TARGET tests_net_prometheus_http PROPERTY LINK_LIBRARIES ${tests_net_prometheus_http_libraries})

  add_test(NAME prometheus_http COMMAND tests_net_prometheus_http)
endif()
