if(DEFINED net_driver_ws_base)
  file(GLOB test_net_driver_ws_source ${net_driver_ws_base}/tests/*.c)

  add_executable(tests_net_driver_ws ${test_net_driver_ws_source})

  set_property(TARGET tests_net_driver_ws PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${wslay_base}/include
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_base}/test_utils
    ${net_core_base}/include
    ${net_core_base}/test_utils
    ${net_driver_ws_base}/include
    ${net_driver_ws_base}/src
    ${net_driver_ws_base}/test_utils
    )

  set(tests_net_driver_ws_libraries
    test_utils_cpe_utils test_utils_net_core test_utils_net_driver_ws
    net_driver_ws net_core  cpe_utils_sock cpe_utils cpe_pal
    wslay cmocka)

  # if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64)
  #   set(tests_net_driver_ws_libraries ${tests_net_driver_ws_libraries} m)
  # endif()

  set_property(TARGET tests_net_driver_ws PROPERTY LINK_LIBRARIES ${tests_net_driver_ws_libraries})

  add_test(NAME net-ws COMMAND tests_net_driver_ws)
endif()
