if (DEFINED net_protocol_http_base)
  file(GLOB test_utils_net_protocol_http_source ${net_protocol_http_base}/test_utils/*.c)

  add_library(test_utils_net_protocol_http STATIC ${test_utils_net_protocol_http_source})

  set_property(TARGET test_utils_net_protocol_http PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${net_core_includes}
    ${cpe_utils_base}/test_utils
    ${net_protocol_http_base}/include
    )

  target_link_libraries(test_utils_net_protocol_http INTERFACE net_protocol_http)
endif()
