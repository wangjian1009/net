if(DEFINED net_protocol_http_base)
  file(GLOB test_net_protocol_http_source ${net_protocol_http_base}/tests/net_http_*.c)

  set(tests_net_protocol_http_libraries
    test_utils_cpe_utils
    test_utils_net_core
    test_utils_net_protocol_http)

  if(DEFINED net_driver_ssl_base)
    file(GLOB test_net_protocol_http_source_https ${net_protocol_http_base}/tests/net_https_*.c)
    set(test_net_protocol_http_source ${test_net_protocol_http_source} ${test_net_protocol_http_source_https})
    
    set(tests_net_protocol_http_compile_definitions
      ${tests_net_protocol_http_compile_definitions} TESTS_NET_PROTOCOL_HTTP_SSL=1)

    set(tests_net_protocol_http_libraries ${tests_net_protocol_http_libraries} net_driver_ssl)
  endif()
  
  add_executable(tests_net_protocol_http ${test_net_protocol_http_source})

  set_property(TARGET tests_net_protocol_http PROPERTY COMPILE_DEFINITIONS ${tests_net_protocol_http_compile_definitions})
  
  set_property(TARGET tests_net_protocol_http PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${openssl_base}/include
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_base}/test_utils
    ${net_core_base}/include
    ${net_core_base}/test_utils
    ${net_driver_ssl_base}/include
    ${net_protocol_http_base}/include
    ${net_protocol_http_base}/test_utils
    ${net_protocol_http_base}/src
    )

  set_property(TARGET tests_net_protocol_http PROPERTY LINK_LIBRARIES ${tests_net_protocol_http_libraries})

  add_test(NAME net-protocol-http COMMAND tests_net_protocol_http)
endif()
