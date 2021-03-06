if(DEFINED net_ndt7_base)
  file(GLOB tests_net_ndt7_source ${net_ndt7_base}/tests/*.c)

  add_executable(tests_net_ndt7 ${tests_net_ndt7_source})

  set_property(TARGET tests_net_ndt7 PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${yajl_base}/include
    ${cpe_utils_json_base}/include
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_base}/test_utils
    ${net_protocol_http_base}/include
    ${net_protocol_http_base}/test_utils
    ${net_protocol_http_svr_base}/include
    ${net_protocol_http_svr_base}/test_utils
    ${net_driver_ws_base}/include
    ${net_driver_ws_base}/test_utils
    ${net_core_base}/include
    ${net_core_base}/test_utils  
    ${net_ndt7_base}/include
    ${net_ndt7_base}/src
    )

  set(tests_net_ndt7_libraries net_ndt7 test_utils_net_core test_utils_net_protocol_http_svr test_utils_net_driver_ws)

  set_property(TARGET tests_net_ndt7 PROPERTY LINK_LIBRARIES ${tests_net_ndt7_libraries})

  add_test(NAME net-ndt7 COMMAND tests_net_ndt7)
endif()
