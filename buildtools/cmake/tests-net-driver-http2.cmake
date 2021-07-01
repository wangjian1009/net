if (DEFINED net_driver_http2_base)
  file(GLOB test_net_driver_http2_source ${net_driver_http2_base}/tests/*.c)

  add_executable(tests_net_driver_http2 ${test_net_driver_http2_source})

  set_property(TARGET tests_net_driver_http2 PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${nghttp2_base}/lib/includes
    ${nghttp2_custom}
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_base}/test_utils
    ${net_core_base}/include
    ${net_core_base}/test_utils
    ${net_driver_http2_base}/include
    ${net_driver_http2_base}/src
    )

  set(tests_net_driver_http2_libraries
    test_utils_cpe_utils test_utils_net_core
    net_driver_http2 net_core cpe_utils_sock cpe_utils cpe_pal
    cmocka)

  # if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64)
  #   set(tests_net_driver_http2_libraries ${tests_net_driver_http2_libraries} m)
  # endif()

  set_property(TARGET tests_net_driver_http2 PROPERTY LINK_LIBRARIES ${tests_net_driver_http2_libraries})

  add_test(NAME net-http2 COMMAND tests_net_driver_http2)
endif()
